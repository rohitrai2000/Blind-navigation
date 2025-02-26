// This file is part of Eigen, a lightweight C++ template library
// for linear algebra.
//
// Copyright (C) 2015 Gael Guennebaud <gael.guennebaud@inria.fr>
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "main.h"

void check(bool b, bool ref)
{
  std::cout << b;
  if(b==ref)
    std::cout << " OK  ";
  else
    std::cout << " BAD ";
}

#if EIGEN_COMP_MSVC && EIGEN_COMP_MSVC < 1800
namespace std {
  template<typename T> bool (isfinite)(T x) { return _finite(x); }
  template<typename T> bool (isnan)(T x) { return _isnan(x); }
  template<typename T> bool (isinf)(T x) { return _fpclass(x)==_FPCLASS_NINF || _fpclass(x)==_FPCLASS_PINF; }
}
#endif

template<typename T>
void check_inf_nan(bool dryrun) {
  Matrix<T,Dynamic,1> m(10);
  m.setRandom();
  m(3) = std::numeric_limits<T>::quiet_NaN();

  if(dryrun)
  {
    std::cout << "std::isfinite(" << m(3) << ") = "; check((std::isfinite)(m(3)),false); std::cout << "  ; numext::isfinite = "; check((numext::isfinite)(m(3)), false); std::cout << "\n";
    std::cout << "std::isinf(" << m(3) << ")    = "; check((std::isinf)(m(3)),false);    std::cout << "  ; numext::isinf    = "; check((numext::isinf)(m(3)), false); std::cout << "\n";
    std::cout << "std::isnan(" << m(3) << ")    = "; check((std::isnan)(m(3)),true);     std::cout << "  ; numext::isnan    = "; check((numext::isnan)(m(3)), true); std::cout << "\n";
    std::cout << "allFinite: "; check(m.allFinite(), 0); std::cout << "\n";
    std::cout << "hasNaN:    "; check(m.hasNaN(), 1);    std::cout << "\n";
    std::cout << "\n";
  }
  else
  {
    VERIFY( !(numext::isfinite)(m(3)) );
    VERIFY( !(numext::isinf)(m(3)) );
    VERIFY(  (numext::isnan)(m(3)) );
    VERIFY( !m.allFinite() );
    VERIFY(  m.hasNaN() );
  }
  m(4) /= T(0.0);
  if(dryrun)
  {
    std::cout << "std::isfinite(" << m(4) << ") = "; check((std::isfinite)(m(4)),false); std::cout << "  ; numext::isfinite = "; check((numext::isfinite)(m(4)), false); std::cout << "\n";
    std::cout << "std::isinf(" << m(4) << ")    = "; check((std::isinf)(m(4)),true);     std::cout << "  ; numext::isinf    = "; check((numext::isinf)(m(4)), true); std::cout << "\n";
    std::cout << "std::isnan(" << m(4) << ")    = "; check((std::isnan)(m(4)),false);    std::cout << "  ; numext::isnan    = "; check((numext::isnan)(m(4)), false); std::cout << "\n";
    std::cout << "allFinite: "; check(m.allFinite(), 0); std::cout << "\n";
    std::cout << "hasNaN:    "; check(m.hasNaN(), 1);    std::cout << "\n";
    std::cout << "\n";
  }
  else
  {
    VERIFY( !(numext::isfinite)(m(4)) );
    VERIFY(  (numext::isinf)(m(4)) );
    VERIFY( !(numext::isnan)(m(4)) );
    VERIFY( !m.allFinite() );
    VERIFY(  m.hasNaN() );
  }
  m(3) = 0;
  if(dryrun)
  {
    std::cout << "std::isfinite(" << m(3) << ") = "; check((std::isfinite)(m(3)),true); std::cout << "  ; numext::isfinite = "; check((numext::isfinite)(m(3)), true); std::cout << "\n";
    std::cout << "std::isinf(" << m(3) << ")    = "; check((std::isinf)(m(3)),false);    std::cout << "  ; numext::isinf    = "; check((numext::isinf)(m(3)), false); std::cout << "\n";
    std::cout << "std::isnan(" << m(3) << ")    = "; check((std::isnan)(m(3)),false);     std::cout << "  ; numext::isnan    = "; check((numext::isnan)(m(3)), false); std::cout << "\n";
    std::cout << "allFinite: "; check(m.allFinite(), 0); std::cout << "\n";
    std::cout << "hasNaN:    "; check(m.hasNaN(), 0);    std::cout << "\n";
    std::cout << "\n\n";
  }
  else
  {
    VERIFY(  (numext::isfinite)(m(3)) );
    VERIFY( !(numext::isinf)(m(3)) );
    VERIFY( !(numext::isnan)(m(3)) );
    VERIFY( !m.allFinite() );
    VERIFY( !m.hasNaN() );
  }
}

void test_fastmath() {
  std::cout << "*** float *** \n\n"; check_inf_nan<float>(true);
  std::cout << "*** double ***\n\n"; check_inf_nan<double>(true);
  std::cout << "*** long double *** \n\n"; check_inf_nan<long double>(true);

  check_inf_nan<float>(false);
  check_inf_nan<double>(false);
  check_inf_nan<long double>(false);
}
