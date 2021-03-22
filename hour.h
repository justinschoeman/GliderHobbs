/*
 * Event driven hour meter
 * 
 * internal ticks are 0.01H 36s/36000ms
 * 
 * 16 bit = 655.36 (max continuous ticks on internal hour
 * 
 */

#include <avr/eeprom.h>
class Hour {
  private:
    uint16_t ee_base; // eeprom base address
    uint8_t ee_blocks; // total eeprom blocks
    uint8_t cur_block; // current block
    uint32_t base_ticks; // ticks from previous blocks
    uint16_t ticks; // ticks from current block
    uint16_t current_ticks; // ticks from current flight
    bool act; // running?
    unsigned long ms; // ms elapsed since last tick
    const unsigned long tick_ms = 36000UL; // ms per tick
    const uint16_t block_max = 50000UL; // maximum ticks (writes) per block
    union {
      uint16_t w[2];
      uint8_t b[4];
    } buffer; // read/write buffer
    int8_t w_state; // async write state (byte no, <0 for hold)
    uint8_t w_block; // async write to block

    // verify and attempt to recover a read block
    // return true if the value was recovered
    // return false if the value was correct as-is
    // die if value is not recoverable
    bool block_verify(void) {
      // invert the second block - in the ideal case w1 is now what should be in w0 (or was in before partial update)
      buffer.w[1] = ~buffer.w[1];
      // perfect case
      if(buffer.w[0] == buffer.w[1]) return 0;
      // write could theoretically fail at any point
      // if we fail after one byte, either
      // 1.1 only LSB is updated, and w0 is 1 more than w1 (simplified test here to cover (2) too)
      if(buffer.w[0] == buffer.w[1] + 1) return 1;
      // 1.2 LSB has changed from 255 to 0 without MSB updating
      if(buffer.b[0] == 0 && buffer.b[2] == 255 && buffer.b[1] == buffer.b[3]) {
        buffer.w[0] = buffer.w[1] + 1; // correct value should be the increment of the unmodified value
        return 1;
      }
      // 2 both bytes of first word are written - w[0] == w[1]+1
      // already covered in test (1.1)
      // 3.1 only LSB is of 2nd word is updated and msb is unchanged
      // will match perfect test
      // 3.2 LSB of 2nd word changed from 255 to 0 without MSB updating
      if(buffer.b[0] == 0 && buffer.b[2] == 0 and buffer.b[1] == buffer.b[3]+1) return 1;
      // anything else is unrecoverable...
#ifdef DEBUG
      Serial.print("EEPROM Corrupt at base : ");
      Serial.print(ee_base);
      Serial.print(" ");
      Serial.print(buffer.w[0], HEX);
      Serial.print(" ");
      Serial.println(buffer.w[1], HEX);
#endif
      display_error(F("EEPROM CORRUPT"));
    }

    // read and verify block from eeprom
    uint16_t ee_read(uint8_t block) {
      eeprom_read_block((void *)(buffer.w), (void*)(ee_base + 4 * block), 4);
#ifdef DEBUG
      Serial.print("read block: ");
      Serial.print(block);
      Serial.println(" ");
      Serial.print(buffer.w[0], BIN);
      Serial.print(" ");
      Serial.println(buffer.w[1], BIN);
#endif
      // verify/correct the block data - does not return if corrupt
      if(block_verify()) {
        // if verification needed to error correct the block, write the corrected block back...
        ee_write(block, buffer.w[0], 1);
      }
      return buffer.w[0];
    }

    // write value to eeprom (or schedule write in update() if now is false)
    void ee_write(uint8_t block, uint16_t val, bool now) {
#ifdef DEBUG
      Serial << "Write at base " << (int)ee_base << ", block " << block << " value: ";
      Serial.println(val);
#endif
      buffer.w[0] = val;
      buffer.w[1] = ~val;
      if(!now) {
        w_state = 0;
        w_block = block;
        return;
      }
      eeprom_update_block((void*)buffer.w, (void*)(ee_base + 4 * block), 4);
    }
    
  public:
    
    Hour(uint16_t base, uint8_t blocks) {
      ee_base = base;
      ee_blocks = blocks;
      act = 0;
      w_state = -1;
    }

    // load the timer from the eeprom value
    void load(void) {
      uint16_t tmp;
      
#if 0
      // block verify testing
      union {
        uint16_t w[2];
        uint8_t b[4];
      } bt;
      uint16_t i;
      int j;
      int k;
      for(i = 0; i < block_max; i++) {
        for(j = 1; j <= 4; j++) {
          Serial.print(i);
          Serial.print(" ");
          Serial.println(j);
          buffer.w[0] = i;
          buffer.w[1] = ~i;
          bt.w[0] = i+1;
          bt.w[1] = ~bt.w[0];
          for(k = 0; k < j; k++) buffer.b[k] = bt.b[k];
          if(block_verify()) {
            Serial.println("corrected");
          }
          Serial.println(buffer.w[0]);
          if(buffer.w[0] != i+1) {
            Serial.println("fail");
            while(1) {}
          }
        }
      }
#endif

      // read initial state from eeprom
      for(cur_block = 0; cur_block < ee_blocks ; cur_block++) {
        tmp = ee_read(cur_block);
        if(tmp > block_max) {
          display_error(F("EEPROM BLOCK TOO BIG"));
        }
        if(tmp < block_max) {
          ticks = tmp;
          return;
        }
        base_ticks += tmp;
      }
      display_error(F("EEPROM LOAD OVERFLOW"));
    }

    // initialise the time in ticks
    // always call load afterwards!
    void set(uint32_t ticks) {
      uint16_t tmp;
      for(cur_block = 0; cur_block < ee_blocks ; cur_block++) {
        tmp = ticks > block_max ? block_max : ticks;
        ee_write(cur_block, tmp, 1);
        ticks -= tmp;
      }
      if(ticks) {
        display_error(F("EEPROM SET OVERFLOW"));
      }
    }

    // start/restart the timer running
    void start(void) {
      // new start reference
      ms = millis();
      // reset current flight
      current_ticks = 0;
      // set active flag
      act = 1;
    }

    // stop/pause the timer
    void stop(void) {
      // simply clear active flag - remaining state set on start
      act = 0;
      // no need to write, as write is synchronous to the ticks.
      // may lose at most 36s...
    }

    // run the state engine
    // return 1 is any significant time was (possibly) used, otherwise 0
    int update(void) {

      // if we are writing, complete write before doing any other updates
      if(w_state >= 0) {
        // eeprom ready? if not, return.
        if(!eeprom_is_ready()) return 0; // do not block - wait until EEPROM is ready for write before we try to write...
        // write current byte
#ifdef DEBUG
        Serial << "Write at base " << (int)ee_base << ", block " << w_block << " offset " << w_state << " value: " << buffer.b[w_state] << "\n";
#endif
        eeprom_update_byte((uint8_t*)(ee_base + 4 * w_block + w_state), buffer.b[w_state]);
        // get next byte
        w_state++;
        // or done
        if(w_state >= 4) w_state = -1;
        return 1;
      }

      // if timer not running, return now
      if(!act) return 0;

      // consume any elapsed tick since last execution (one tick at a time, so eeprom writes are sequential)
      if((millis() - ms) > tick_ms) {
        // consume 1 tick worth of ms
        ms += tick_ms;
        ticks++;
        current_ticks++;
        // if we advanced to the next 500 hour block, then update state accordingly
        if(ticks > block_max) {
          cur_block++;
          if(cur_block >= ee_blocks) {
            display_error(F("EEPROM UPDATE OVERFLOW"));
          }
          // this was the first tick of the next block
          ticks = 1;
          base_ticks += block_max;
        }
        // schedule the write
        ee_write(cur_block, ticks, 0);
      }
      return 0;
    }

    // get current total time
    float get(void) {
      return (float)(base_ticks + ticks)/100.0;
    }

    // get current flight time
    float get_current(void) {
      return (float)(current_ticks)/100.0;
    }
};
