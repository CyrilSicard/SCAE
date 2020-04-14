/*
    Example implementation of an Elliptic Curve over a Finite Field
    By Jarl Ostensen, December 2007
    jarl.ostensen@gmail.com
    
    I wrote this because I wanted to understand more about Elliptic Curves and how they are 
    used in cryoptography. There's plenty of sources and articles out there but I didn't find anything that 
    condenced it all down to a "one pager" of code...
    And as I completed this I thought others might want find it useful to help understand how ECC "works". 
    
    So, if you scroll all the way down to the bottom you'll find a symmmetric encryption example using 
    ECC that motivates pretty much all the code above it...
    
    DISCLAIMERS:
    * I obviously take no responsibility for any state secrets you might loose should 
      you actually *use* the code herein...    
    * I have written this as a fun little intellectual excercise - there might be mistakes, there might be bugs...

    Main sources:
        * Certicom: http://www.certicom.com/index.php?action=ecc,home
        * Wikipedia
        * Harry J. Smith's inverse-modulo implementation 
            http://www.geocities.com/hjsmithh/Numbers/InvMod.html
        * Raju and Akbani: http://www.cs.utsa.edu/~rakbani/publications/Akbani-ECC-IEEESMC03.pdf
        * Allardyce and Goyal: http://www.ece.tamu.edu/~reddy/ee689_04/pres_joel_nitesh.pdf
        * Klaus Reinhard's ECC test page: http://www-fs.informatik.uni-tuebingen.de/~reinhard/krypto/English/4.4.en.html

    Developed using Dev-C++ version 4.9.9.2
    http://www.bloodshed.net
*/

#include <cstdlib>
#include <iostream>
#include <vector>
#include <math.h>
#include "FiniteFieldElement.hpp"

namespace Cryptography
{
    namespace detail {

        int EGCD(int a, int b, int& u, int &v)
        {
            u = 1;
            v = 0;
            int g = a;
            int u1 = 0;
            int v1 = 1;
            int g1 = b;
            while (g1 != 0)
            {
                int q = g/g1; // Integer divide
                int t1 = u - q*u1;
                int t2 = v - q*v1;
                int t3 = g - q*g1;
                u = u1; v = v1; g = g1;
                u1 = t1; v1 = t2; g1 = t3;
            }

            return g;
        }

        int InvMod(int x, int n)
        {
            //n = Abs(n);
            x = x % n; // % is the remainder function, 0 <= x % n < |n|
            int u,v,g,z;
            g = EGCD(x, n, u,v);
            if (g != 1)
            {
                // x and n have to be relative prime for there to exist an x^-1 mod n
                z = 0;
            }
            else
            {
                z = u % n;
            }
            return z;
        }
    }

    template<int P>
    EllipticCurve<P>::EllipticCurve(int a, int b)
    : a_(a),
      b_(b),
      m_table_(),
      table_filled_(false)
    {
    }

    template<int P>
    void    EllipticCurve<P>::CalculatePoints()
    {
        int x_val[P];
        int y_val[P];
        for ( int n = 0; n < P; ++n )
        {
            int nsq = n*n;
            x_val[n] = ((n*nsq) + a_.i() * n + b_.i()) % P;
            y_val[n] = nsq % P;
        }

        for ( int n = 0; n < P; ++n )
        {
            for ( int m = 0; m < P; ++m )
            {
                if ( x_val[n] == y_val[m] )
                {
                    m_table_.push_back(Point(n,m,*this));
                }
            }
        }

        table_filled_ = true;
    }

    template<int P>
    typename EllipticCurve<P>::Point   EllipticCurve<P>::operator[](int n)
    {
        if ( !table_filled_ )
        {
            CalculatePoints();
        }

        return m_table_[n];
    }

    template<int P>
    void    EllipticCurve<P>::Point::addDouble(int m, Point& acc)
    {
        if ( m > 0 )
        {
            Point r = acc;
            for ( int n=0; n < m; ++n )
            {
                r += r;     // doubling step
            }
            acc = r;
        }
    }

    template<int P>
    typename EllipticCurve<P>::Point    EllipticCurve<P>::Point::scalarMultiply(int k, const Point& a)
    {
        Point acc = a;
        Point res = Point(0, 0, *ec_);
        int i = 0, j = 0;
        int b = std::abs(k); // Infinite loop for negative numbers.

        while( b != 0 )
        {
            if ( b & 1 )
            {
                // bit is set; acc = 2^(i-j)*acc
                addDouble(i - j, acc);
                res += acc;
                j = i;  // last bit set
            }
            b >>= 1;
            ++i;
        }
        return res;
    }

    template<int P>
    void    EllipticCurve<P>::Point::add(ffe_t x1, ffe_t y1, ffe_t x2, ffe_t y2, ffe_t & xR, ffe_t & yR) const
    {
         // special cases involving the additive identity
        if ( x1 == 0 && y1 == 0 )
        {
            xR = x2;
            yR = y2;
            return;
        }
        if ( x2 == 0 && y2 == 0 )
        {
            xR = x1;
            yR = y1;
            return;
        }
        if ( y1 == -y2 )
        {
            xR = yR = 0;
            return;
        }

        // the additions
        ffe_t s;
        if ( x1 == x2 && y1 == y2 )
        {
            //2P
            s = (3*(x1.i()*x1.i()) + ec_->a()) / (2*y1);
            xR = ((s*s) - 2*x1);
        }
        else
        {
            //P+Q
            s = (y1 - y2) / (x1 - x2);
            xR = ((s*s) - x1 - x2);
        }

        if ( s != 0 )
        {
            yR = (-y1 + s*(x1 - xR));
        }
        else
        {
            xR = yR = 0;
        }
    }

    template<int P>
    unsigned int     EllipticCurve<P>::Point::Order(unsigned int maxPeriod) const
    {
        Point r = *this;
        unsigned int n = 0;
        while( r.x_ != 0 && r.y_ != 0 )
        {
            ++n;
            r += *this;
            if ( n > maxPeriod ) break;
        }
        return n;
    }
                               
    template<int T>
    std::ostream& operator<<(std::ostream& os, const EllipticCurve<T>& EllipticCurve)
    {
        os << "y^2 mod " << T << " = (x^3" << std::showpos;
        if ( EllipticCurve.a_ != 0 )
        {
            os << EllipticCurve.a_ << "x";
        }

        if ( EllipticCurve.b_.i() != 0 )
        {
            os << EllipticCurve.b_;
        }

        os << std::noshowpos << ") mod " << T;
        return os;
    }

    template<int P>
    std::ostream&    EllipticCurve<P>::PrintTable(std::ostream &os, int columns)
    {
        if ( table_filled_ )
        {
            int col = 0;
            typename EllipticCurve<P>::m_table_t::iterator iter = m_table_.begin();
            for ( ; iter!=m_table_.end(); ++iter )
            {
                os << "(" << (*iter).x_.i() << ", " << (*iter).y_.i() << ") ";
                if ( ++col > columns )
                {
                    os << "\n";
                    col = 0;
                }
            }
        }
        else
        {
            os << "EllipticCurve, F_" << P;
        }
        return os;
    }
}

namespace   utils
{    
    float   frand()
    {
        static float norm = 1.0f / (float)RAND_MAX;
        return (float)rand()*norm;
    }
    
    int irand(int min, int max)
    {
        return min+(int)(frand()*(float)(max-min));
    }
}
