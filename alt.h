float alt_alt = 0.0;
float alt_vsi = 0.0;

inline float alt_p2a(float p) {
  const float k_a = /*3.28084 **/ 44330.0; // m to ft
  const float k_e = 1.0/5.255;
  const float k_p0 = 101325.0;
  return k_a * (1 - pow(p/k_p0, k_e));
}

//https://github.com/rfetick/Kalman
#include "Kalman.h"
using namespace BLA;

//------------------------------------
/****  MODELIZATION PARAMETERS  ****/
//------------------------------------

#define ALT_Nstate 2 // position, speed
#define ALT_Nobs 1   // position

// measurement std (3 sigma is around 2 ft)
//#define n_p (60.0 / 3.0)
//#define ALT_n_p 2.4693
//#define ALT_n_p 2.1093e-01
#define ALT_n_p (2.1093e-01 * 8.0)
// model std (1/inertia)
#define ALT_m_p 0.1
#define ALT_m_s 0.1


BLA::Matrix<ALT_Nobs> alt_obs; // observation vector
KALMAN<ALT_Nstate, ALT_Nobs> alt_K; // your Kalman filter

int alt_init = 0;
unsigned long alt_time;

void alt_setup(void) {
}

int _alt_init(float a) {
    // init kalman
    alt_time = millis();
    // initial state
    alt_K.x = {a, 0.0};
    // measurement matrix
    alt_K.H = {
        1.0,  0.0
    };
    // measurement covariance matrix
    alt_K.R = {
        ALT_n_p*ALT_n_p
    };
    // model covariance matrix
#if 0
    alt_K.Q = {
        m_p*m_p,  0.0,
        0.0,      m_s*m_s
    };
#else
    const float dt = (1.0/50.0); // /60.0;
    const float g = 9.8; //115826.574; // ft/min/min
    const float sg = (1.0*g)/3.0;
    const float dt2sg2 = dt * dt * sg * sg;
    alt_K.Q = {
        dt2sg2*dt*dt*0.25,  dt2sg2*dt*0.5,
        dt2sg2*dt*0.5,      dt2sg2
    };
#endif
    alt_init = 1;
    return 1;
}

int alt_run(float p) {
  float a = alt_p2a(p);

#ifdef DEBUG
  Serial.println(a);
#endif

  // init if required
  if(!alt_init) return _alt_init(a);

  // update kalman
  unsigned long t = millis();
  alt_time = t - alt_time;
  alt_K.F = {
      1.0,  ((float)alt_time) / 1000.0,
      0.0,  1.0
  };
  //Serial << K.F * 1000.0 << "\n";
  alt_time = t;
  alt_obs(0) = a;
  // APPLY KALMAN FILTER
  alt_K.update(alt_obs);
  //Serial << alt_K.F << "\n";
  //Serial << alt_K.x << "\n";
  alt_alt = alt_K.x(0);
  alt_vsi = alt_K.x(1);
}
