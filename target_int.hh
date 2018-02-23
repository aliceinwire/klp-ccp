#ifndef TARGET_INT_HH
#define TARGET_INT_HH

#include "mp_arithmetic.hh"

namespace suse
{
  namespace cp
  {
    class target_int
    {
    public:
      target_int(const mpa::limbs::size_type prec, const bool is_signed,
		 mpa::limbs &&ls);

      target_int(const target_int &ti);
      target_int(target_int &&ti);

      bool operator==(const target_int &op) const noexcept;

      bool operator!=(const target_int &op) const noexcept
      {
	return !(*this == op);
      }

      bool operator<(const target_int &op) const noexcept;

      bool operator>(const target_int &op) const noexcept
      {
	return op < *this;
      }

      bool operator<=(const target_int &op) const noexcept;

      bool operator>=(const target_int &op) const noexcept
      {
	return op <= *this;
      }

      explicit operator bool() const noexcept;

      target_int operator-() const;
      target_int operator~() const;

      target_int operator+(const target_int &op) const;
      target_int operator-(const target_int &op) const;
      target_int operator*(const target_int &op) const;
      target_int operator/(const target_int &op) const;
      target_int operator%(const target_int &op) const;

      target_int operator^(const target_int &op) const;
      target_int operator&(const target_int &op) const;
      target_int operator|(const target_int &op) const;

      target_int operator<<(const target_int &op) const;
      target_int operator>>(const target_int &op) const;

      target_int convert(const unsigned int prec, const bool is_signed) const;

      const mpa::limbs& get_limbs() const noexcept
      {
	return _limbs;
      }

      bool is_negative() const noexcept;

    private:
      void _assert_same_prec_and_signedness(const target_int &op)
	const noexcept;

      mpa::limbs::size_type _width() const noexcept
      {
	return _prec + _is_signed;
      }

      mpa::limbs::size_type _n_limbs() const noexcept;

      bool _is_negative() const noexcept;

      void _propagate_sign_to_high(mpa::limbs &ls) const noexcept;
      void _clamp_unsigned(mpa::limbs &ls) const noexcept;

      mpa::limbs::size_type _to_size_type() const;

      const mpa::limbs::size_type _prec;
      const bool _is_signed;
      const mpa::limbs _limbs;
    };
  }
}

#endif
