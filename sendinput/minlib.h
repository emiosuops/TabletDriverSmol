#pragma once
void* memcpy(void* dest, const void* src, size_t count)
{
	/* This would be a prime candidate for reimplementation in assembly */
	char* in_src = (char*)src;
	char* in_dest = (char*)dest;

	while (count--)
		*in_dest++ = *in_src++;
	return dest;
}
int my_isdigit(int c)
{
	return ((c >= '0') && (c <= '9'));
}
int my_isspace(int c)
{
	return ((c == ' ') || (c == '\n') || (c == '\t'));
}
int my_isupper(int ch)
{
	return (ch >= 'A' && ch <= 'Z');
}
int myisalpha(int c)
{
	char alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"abcdefghijklmnopqrstuvwxyz";
	char* letter = alphabet;

	while (*letter != '\0' && *letter != c)
		++letter;

	if (*letter)
		return 1;

	return 0;
}
long
a_strtol(const char* nptr, char** endptr, register int base)
{
	register const char* s = nptr;
	register long acc;
	register int c;
	register unsigned long cutoff;
	register int neg = 0, any, cutlim;

	/*
	 * Skip white space and pick up leading +/- sign if any.
	 * If base is 0, allow 0x for hex and 0 for octal, else
	 * assume decimal; if base is already 16, allow 0x.
	 */
	do {
		c = *s++;
	} while (my_isspace(c));
	if (c == '-') {
		neg = 1;
		c = *s++;
	}
	else if (c == '+')
		c = *s++;
	if ((base == 0 || base == 16) &&
		c == '0' && (*s == 'x' || *s == 'X')) {
		c = s[1];
		s += 2;
		base = 16;
	}
	if (base == 0)
		base = c == '0' ? 8 : 10;

	/*
	 * Compute the cutoff value between legal numbers and illegal
	 * numbers.  That is the largest legal value, divided by the
	 * base.  An input number that is greater than this value, if
	 * followed by a legal input character, is too big.  One that
	 * is equal to this value may be valid or not; the limit
	 * between valid and invalid numbers is then based on the last
	 * digit.  For instance, if the range for longs is
	 * [-2147483648..2147483647] and the input base is 10,
	 * cutoff will be set to 214748364 and cutlim to either
	 * 7 (neg==0) or 8 (neg==1), meaning that if we have accumulated
	 * a value > 214748364, or equal but the next digit is > 7 (or 8),
	 * the number is too big, and we will return a range error.
	 *
	 * Set any if any `digits' consumed; make it negative to indicate
	 * overflow.
	 */
	cutoff = neg ? -(long)LONG_MIN : LONG_MAX;
	cutlim = cutoff % (unsigned long)base;
	cutoff /= (unsigned long)base;
	for (acc = 0, any = 0;; c = *s++) {
		if (my_isdigit(c))
			c -= '0';
		else if (myisalpha(c))
			c -= my_isupper(c) ? 'A' - 10 : 'a' - 10;
		else
			break;
		if (c >= base)
			break;
		if (any < 0 || acc > cutoff || (acc == cutoff && c > cutlim))
			any = -1;
		else {
			any = 1;
			acc *= base;
			acc += c;
		}
	}
	if (any < 0) {
		acc = neg ? LONG_MIN : LONG_MAX;
	}
	else if (neg)
		acc = -acc;
	if (endptr != 0)
		*endptr = (char*)(any ? s - 1 : nptr);
	return (acc);
}