#pragma once

#include <ostream>
#include <vector>

namespace Cryptography
{
       // helper functions
        namespace   detail
        {            
            //From Knuth; Extended GCD gives g = a*u + b*v
            int EGCD(int a, int b, int& u, int &v);
            
            int InvMod(int x, int n); // Solve linear congruence equation x * z == 1 (mod n) for z
        }
        
        /*
            An element in a Galois field FP
            Adapted for the specific behaviour of the "mod" function where (-n) mod m returns a negative number
            Allows basic arithmetic operations between elements:
                +,-,/,scalar multiply                
                
            The template argument P is the order of the field
        */
        template<int P>
        class   FiniteFieldElement
        {
            int     i_;
            
            void    assign(int i)
            {
                i_ = i;
                if ( i<0 )
                {                    
                    // ensure (-i) mod p correct behaviour
                    // the (i%P) term is to ensure that i is in the correct range before normalizing
                    i_ = (i%P) + 2*P;
                }

                i_ %= P;
            }
            
            public:
                // ctor
                FiniteFieldElement()
                 : i_(0)
                {}
                // ctor
                explicit FiniteFieldElement(int i)
                {
                    assign(i);
                }
                // copy ctor
                FiniteFieldElement(const FiniteFieldElement<P>& rhs) 
                 : i_(rhs.i_)               
                {
                }
                
                // access "raw" integer
                int i() const { return i_; }                
                // negate
                FiniteFieldElement  operator-() const
                {
                    return FiniteFieldElement(-i_);
                }                                
                // assign from integer
                FiniteFieldElement& operator=(int i)
                {
                    assign(i);
                    return *this;
                }
                // assign from field element
                FiniteFieldElement<P>& operator=(const FiniteFieldElement<P>& rhs)
                {
                    i_ = rhs.i_;
                    return *this;
                }
                // *=
                FiniteFieldElement<P>& operator*=(const FiniteFieldElement<P>& rhs)
                {
                    i_ = (i_*rhs.i_) % P;
                    return *this;           
                }
                // ==
                friend bool    operator==(const FiniteFieldElement<P>& lhs, const FiniteFieldElement<P>& rhs)
                {
                    return (lhs.i_ == rhs.i_);
                }
                // == int
                friend bool    operator==(const FiniteFieldElement<P>& lhs, int rhs)
                {
                    return (lhs.i_ == rhs);
                }
                // !=
                friend bool    operator!=(const FiniteFieldElement<P>& lhs, int rhs)
                {
                    return (lhs.i_ != rhs);
                }                
                // a / b
                friend FiniteFieldElement<P> operator/(const FiniteFieldElement<P>& lhs, const FiniteFieldElement<P>& rhs)
                {
                    return FiniteFieldElement<P>( lhs.i_ * detail::InvMod(rhs.i_,P));
                }
                // a + b
                friend FiniteFieldElement<P> operator+(const FiniteFieldElement<P>& lhs, const FiniteFieldElement<P>& rhs)
                {
                    return FiniteFieldElement<P>( lhs.i_ + rhs.i_);
                }
                // a - b
                friend FiniteFieldElement<P> operator-(const FiniteFieldElement<P>& lhs, const FiniteFieldElement<P>& rhs)
                {
                    return FiniteFieldElement<P>( lhs.i_ - rhs.i_);
                }
                // a + int
                friend FiniteFieldElement<P> operator+(const FiniteFieldElement<P>& lhs, int i)
                {
                    return FiniteFieldElement<P>( lhs.i_+i);
                }
                // int + a
                friend FiniteFieldElement<P> operator+(int i, const FiniteFieldElement<P>& rhs)
                {
                    return FiniteFieldElement<P>( rhs.i_+i);
                }
                // int * a
                friend FiniteFieldElement<P> operator*(int n, const FiniteFieldElement<P>& rhs)
                {
                    return FiniteFieldElement<P>( n*rhs.i_);
                }                
                // a * b
                friend FiniteFieldElement<P> operator*(const FiniteFieldElement<P>& lhs, const FiniteFieldElement<P>& rhs)
                {
                    return FiniteFieldElement<P>( lhs.i_ * rhs.i_);
                }
                // ostream handler
                template<int T>
                friend  std::ostream&    operator<<(std::ostream& os, const FiniteFieldElement<T>& g)
                {
                    return os << g.i_;
                }
        };

        /*
            Elliptic Curve over a finite field of order P:
            y^2 mod P = x^3 + ax + b mod P

            NOTE: this implementation is simple and uses normal machine integers for its calculations.
                  No special "big integer" manipulation is supported so anything _big_ won't work.
                  However, it is a complete finite field EC implementation in that it can be used
                  to learn and understand the behaviour of these curves and in particular to experiment with them
                  for their use in cryptography

            Template parameter P is the order of the finite field Fp over which this curve is defined
        */
        template<int P>
        class   EllipticCurve
        {
            public:
                // this curve is defined over the finite field (Galois field) Fp, this is the
                // typedef of elements in it
                typedef FiniteFieldElement<P> ffe_t;

                /*
                    A point, or group element, on the EC, consisting of two elements of the field FP
                    Points can only created by the EC instance itself as they have to be
                    elements of the group generated by the EC
                */
                class   Point
                {
                    friend  class   EllipticCurve<P>;
                    typedef FiniteFieldElement<P> ffe_t;
                    ffe_t  x_;
                    ffe_t  y_;
                    EllipticCurve    *ec_;

                    // core of the doubling multiplier algorithm (see below)
                    // multiplies acc by m as a series of "2*acc's"
                    void   addDouble(int m, Point& acc);

                    // doubling multiplier algorithm
                    // multiplies a by k by expanding in multiplies by 2
                    // a is also an accumulator that stores the intermediate results
                    // between the "1s" of the binary form of the input scalar k
                    Point scalarMultiply(int k, const Point& a);

                    // adding two points on the curve
                    void    add(ffe_t x1, ffe_t y1, ffe_t x2, ffe_t y2, ffe_t & xR, ffe_t & yR) const;

                    Point(int x, int y)
                    : x_(x),
                      y_(y),
                      ec_(0)
                    {}

                    Point(int x, int y, EllipticCurve<P> & EllipticCurve)
                     : x_(x),
                       y_(y),
                       ec_(&EllipticCurve)
                    {}

                    Point(const ffe_t& x, const ffe_t& y, EllipticCurve<P> & EllipticCurve)
                     : x_(x),
                       y_(y),
                       ec_(&EllipticCurve)
                    {}

                public:
                    static  Point   ONE;

                    // copy ctor
                    Point(const Point& rhs)
                    {
                        x_ = rhs.x_;
                        y_ = rhs.y_;
                        ec_ = rhs.ec_;
                    }
                    // assignment
                    Point& operator=(const Point& rhs)
                    {
                        x_ = rhs.x_;
                        y_ = rhs.y_;
                        ec_ = rhs.ec_;
                        return *this;
                    }
                    // access x component as element of Fp
                    ffe_t x() const { return x_; }
                    // access y component as element of Fp
                    ffe_t y() const { return y_; }
                    // calculate the order of this point by brute-force additions
                    // WARNING: this can be VERY slow if the period is long and might not even converge
                    // so maxPeriod should probably be set to something sensible...
                    unsigned int     Order(unsigned int maxPeriod = ~0) const;
                    // negate
                    Point   operator-()
                    {
                        return Point(x_,-y_);
                    }
                    // ==
                    friend bool    operator==(const Point& lhs, const Point& rhs)
                    {
                        return (lhs.ec_ == rhs.ec_) && (lhs.x_ == rhs.x_) && (lhs.y_ == rhs.y_);
                    }
                    // !=
                    friend bool    operator!=(const Point& lhs, const Point& rhs)
                    {
                        return (lhs.ec_ != rhs.ec_) || (lhs.x_ != rhs.x_) || (lhs.y_ != rhs.y_);
                    }
                    // a + b
                    friend Point operator+(const Point& lhs, const Point& rhs)
                    {
                        ffe_t xR, yR;
                        lhs.add(lhs.x_,lhs.y_,rhs.x_,rhs.y_,xR,yR);
                        return Point(xR,yR,*lhs.ec_);
                    }
                    // a * int
                    friend  Point operator*(int k, const Point& rhs)
                    {
                        return Point(rhs).operator*=(k);
                    }
                    // +=
                    Point& operator+=(const Point& rhs)
                    {
                        add(x_,y_,rhs.x_,rhs.y_,x_,y_);
                        return *this;
                    }
                    // a *= int
                    Point& operator*=(int k)
                    {
                        return (*this = scalarMultiply(k,*this));
                    }
                    // ostream handler: print this point
                    friend std::ostream& operator <<(std::ostream& os, const Point& p)
                    {
                        return (os << "(" << p.x_ << ", " << p.y_ << ")");
                    }
                };

                // ==================================================== EllipticCurve impl

                typedef EllipticCurve<P> this_t;
                typedef class EllipticCurve<P>::Point point_t;

                // ctor
                // Initialize EC as y^2 = x^3 + ax + b
                EllipticCurve(int a, int b);

                // Calculate *all* the points (group elements) for this EC
                //NOTE: if the order of this curve is large this could take some time...
                void    CalculatePoints();

                // get a point (group element) on the curve
                Point   operator[](int n);

                // number of elements in this group
                std::size_t  Size() const { return m_table_.size(); }

                // the degree P of this EC
                int     Degree() const { return P; }

                // the parameter a (as an element of Fp)
                FiniteFieldElement<P>  a() const { return a_; }

                // the paramter b (as an element of Fp)
                FiniteFieldElement<P>  b() const { return b_; }

                // ostream handler: print this curve in human readable form
                template<int T>
                friend std::ostream& operator <<(std::ostream& os, const EllipticCurve<T>& EllipticCurve);
                // print all the elements of the EC group
                std::ostream&    PrintTable(std::ostream &os, int columns=4);

                private:
                    typedef std::vector<Point>  m_table_t;

                    m_table_t                   m_table_;   // table of points
                    FiniteFieldElement<P>       a_;         // paramter a of the EC equation
                    FiniteFieldElement<P>       b_;         // parameter b of the EC equation
                    bool    table_filled_;                  // true if the table has been calculated
        };

        template<int T>
            typename EllipticCurve<T>::Point EllipticCurve<T>::Point::ONE(0,0);
}

namespace   utils
{
    float   frand();

    int irand(int min, int max);
}

    
