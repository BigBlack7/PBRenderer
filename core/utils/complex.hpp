#pragma once
#include <cmath>
namespace pbrt
{
    struct Complex
    {
        union
        {
            float __a__;
            float __c__;
        };
        union
        {
            float __b__;
            float __d__;
        };
        Complex(float a, float b) : __a__(a), __b__(b) {}
        Complex(float a) : __a__(a), __b__(0) {}
    };

    inline Complex operator+(const Complex &lhs, const Complex &rhs)
    {
        return {lhs.__a__ + rhs.__c__, lhs.__b__ + rhs.__d__};
    }

    inline Complex operator-(const Complex &lhs, const Complex &rhs)
    {
        return {lhs.__a__ - rhs.__c__, lhs.__b__ - rhs.__d__};
    }

    inline Complex operator*(const Complex &lhs, const Complex &rhs)
    {
        return {lhs.__a__ * rhs.__c__ - lhs.__b__ * rhs.__d__, lhs.__a__ * rhs.__d__ + lhs.__b__ * rhs.__c__};
    }

    inline Complex operator*(const Complex &lhs, const float &rhs)
    {
        return {lhs.__a__ * rhs, lhs.__b__ * rhs};
    }

    inline Complex operator/(const Complex &lhs, const Complex &rhs)
    {
        float denom = 1.f / (rhs.__c__ * rhs.__c__ + rhs.__d__ * rhs.__d__);
        return {(lhs.__a__ * rhs.__c__ - lhs.__b__ * rhs.__d__) * denom, (lhs.__b__ * rhs.__c__ - lhs.__a__ * rhs.__d__) * denom};
    }

    inline Complex operator/(const Complex &lhs, const float &rhs)
    {
        return {lhs.__a__ / rhs, lhs.__b__ / rhs};
    }

    inline float norm(const Complex &rhs)
    {
        return std::sqrt(rhs.__a__ * rhs.__a__ + rhs.__b__ * rhs.__b__);
    }

    inline Complex sqrt(const Complex &rhs)
    {
        float rhs_norm = norm(rhs);
        return {std::sqrt(rhs_norm + rhs.__a__) * 0.5f, std::sqrt(rhs_norm - rhs.__a__) * 0.5f};
    }
}