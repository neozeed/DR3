/*
 * Copyright 1991-1998 by Open Software Foundation, Inc. 
 *              All Rights Reserved 
 *  
 * Permission to use, copy, modify, and distribute this software and 
 * its documentation for any purpose and without fee is hereby granted, 
 * provided that the above copyright notice appears in all copies and 
 * that both the copyright notice and this permission notice appear in 
 * supporting documentation. 
 *  
 * OSF DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE 
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
 * FOR A PARTICULAR PURPOSE. 
 *  
 * IN NO EVENT SHALL OSF BE LIABLE FOR ANY SPECIAL, INDIRECT, OR 
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM 
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN ACTION OF CONTRACT, 
 * NEGLIGENCE, OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION 
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. 
 */
/*
 * MkLinux
 */

/*
 * Mach Standalone library does not provide floating point support.
 */

/*
 * Copyright (c) 1988 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by the University of California, Berkeley.  The name of the
 * University may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)doprnt.c	5.35 (Berkeley) 6/27/88";
#endif /* LIBC_SCCS and not lint */

#include <stdarg.h>

#define STATIC static

/* 11-bit exponent (VAX G floating point) is 308 decimal digits */
#define	MAXEXP		308
/* 128 bit fraction takes up 39 decimal digits; max reasonable precision */
#define	MAXFRACT	39

#define	DEFPREC		6

#define	BUF		(MAXEXP+MAXFRACT+1)	/* + decimal point */

#define	PUTC(ch)	(*putc)(putc_arg, ch)

#define	ARG() \
	_ulong = flags&LONGINT ? va_arg(argp, long) : \
	    flags&SHORTINT ? va_arg(argp, short) : va_arg(argp, int);

#define	todigit(c)	((c) - '0')
#define	tochar(n)	((n) + '0')

/* have to deal with the negative buffer count kludge */
#define	NEGATIVE_COUNT_KLUDGE

#define	LONGINT		0x01		/* long integer */
#define	LONGDBL		0x02		/* long double; unimplemented */
#define	SHORTINT	0x04		/* short integer */
#define	ALT		0x08		/* alternate form */
#define	LADJUST		0x10		/* left adjustment */
#define	ZEROPAD		0x20		/* zero (as opposed to blank) pad */
#define	HEXPREFIX	0x40		/* add 0x or 0X prefix */

double modf(double, double *);
int is_infinity(double);
int is_nan(double);

#define INFINITY_STRING "INF"
#define NAN_STRING "NAN"

STATIC  int cvt();
STATIC	char *exponent(), *round();


doprnt(fmt0, argp, radix, putc, putc_arg)
	unsigned char *fmt0;
	va_list argp;
	int		radix;		/* default radix - for '%r' */
 	void		(*putc)();	/* character output */
	char		*putc_arg;	/* argument for putc */
{
	register unsigned char *fmt;	/* format string */
	register int ch;	/* character from fmt */
	register int cnt;	/* return value accumulator */
	register int n;		/* random handy integer */
	register char *t;	/* buffer pointer */
	double _double;		/* double precision arguments %[eEfgG] */
	unsigned long _ulong;		/* integer arguments %[diouxX] */
	int base;		/* base for [diouxX] conversion */
	int dprec;		/* decimal precision in [diouxX] */
	int fieldsz;		/* field size expanded by sign, etc */
	int flags;		/* flags as above */
	int fpprec;		/* `extra' floating precision in [eEfgG] */
	int prec;		/* precision from format (%.3d), or -1 */
	int realsz;		/* field size expanded by decimal precision */
	int size;		/* size of converted field or string */
	int width;		/* width from format (%8d), or 0 */
	char sign;		/* sign prefix (' ', '+', '-', or \0) */
	char softsign;		/* temporary negative sign for floats */
	char *digs;		/* digits for [diouxX] conversion */
	char buf[BUF];		/* space for %c, %[diouxX], %[eEfgG] */

	fmt = fmt0;
	digs = "0123456789abcdef";
	for (cnt = 0;; ++fmt) {
		for (; (ch = *fmt) && ch != '%';
		     ++cnt, ++fmt)
			PUTC(ch);

		if (!ch) {
			return (cnt);
		}

		flags = 0; dprec = 0; fpprec = 0; width = 0;
		prec = -1;
		sign = '\0';

rflag:		switch (*++fmt) {
		case ' ':
			/*
			 * ``If the space and + flags both appear, the space
			 * flag will be ignored.''
			 *	-- ANSI X3J11
			 */
			if (!sign)
				sign = ' ';
			goto rflag;
		case '#':
			flags |= ALT;
			goto rflag;
		case '*':
			/*
			 * ``A negative field width argument is taken as a
			 * - flag followed by a  positive field width.''
			 *	-- ANSI X3J11
			 * They don't exclude field widths read from args.
			 */
			if ((width = va_arg(argp, int)) >= 0)
				goto rflag;
			width = -width;
			/* FALLTHROUGH */
		case '-':
			flags |= LADJUST;
			goto rflag;
		case '+':
			sign = '+';
			goto rflag;
		case '.':
			if (*++fmt == '*')
				n = va_arg(argp, int);
			else {
				n = 0;
				while (isascii(*fmt) && isdigit(*fmt))
					n = 10 * n + todigit(*fmt++);
				--fmt;
			}
			prec = n < 0 ? -1 : n;
			goto rflag;
		case '0':
			/*
			 * ``Note that 0 is taken as a flag, not as the
			 * beginning of a field width.''
			 *	-- ANSI X3J11
			 */
			flags |= ZEROPAD;
			goto rflag;
		case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
			n = 0;
			do {
				n = 10 * n + todigit(*fmt);
			} while (isascii(*++fmt) && isdigit(*fmt));
			width = n;
			--fmt;
			goto rflag;
		case 'L':
			flags |= LONGDBL;
			goto rflag;
		case 'h':
			flags |= SHORTINT;
			goto rflag;
		case 'l':
			flags |= LONGINT;
			goto rflag;
		case 'c':
			*(t = buf) = va_arg(argp, int);
			size = 1;
			sign = '\0';
			goto pforw;
		case 'D':
			flags |= LONGINT;
			/*FALLTHROUGH*/
		case 'd':
		case 'i':
			ARG();
			if ((long)_ulong < 0) {
				_ulong = -_ulong;
				sign = '-';
			}
			base = 10;
			goto number;
		case 'e':
		case 'E':
		case 'f':
		case 'g':
		case 'G':
			_double = va_arg(argp, double);
			/*
			 * don't do unrealistic precision; just pad it with
			 * zeroes later, so buffer size stays rational.
			 */
			if (prec > MAXFRACT) {
				if (*fmt != 'g' && *fmt != 'G' || (flags&ALT))
					fpprec = prec - MAXFRACT;
				prec = MAXFRACT;
			}
			else if (prec == -1)
				prec = DEFPREC;
			/*
			 * softsign avoids negative 0 if _double is < 0 and
			 * no significant digits will be shown
			 */
			if (_double < 0) {
				softsign = '-';
				_double = -_double;
			}
			else
				softsign = 0;
			/*
			 * cvt may have to round up past the "start" of the
			 * buffer, i.e. ``intf("%.2f", (double)9.999);'';
			 * if the first char isn't NULL, it did.
			 */
			*buf = '\0';
			size = cvt(_double, prec, flags, &softsign, *fmt, buf,
			    buf + sizeof(buf));
			if (softsign)
				sign = '-';
			t = *buf ? buf : buf + 1;
			goto pforw;
		case 'n':
			if (flags & LONGINT)
				*va_arg(argp, long *) = cnt;
			else if (flags & SHORTINT)
				*va_arg(argp, short *) = cnt;
			else
				*va_arg(argp, int *) = cnt;
			break;
		case 'O':
			flags |= LONGINT;
			/*FALLTHROUGH*/
		case 'o':
			ARG();
			base = 8;
			goto nosign;
		case 'p':
			/*
			 * ``The argument shall be a pointer to void.  The
			 * value of the pointer is converted to a sequence
			 * of printable characters, in an implementation-
			 * defined manner.''
			 *	-- ANSI X3J11
			 */
			/* NOSTRICT */
			_ulong = (unsigned long)va_arg(argp, void *);
			base = 16;
			goto nosign;
		case 's':
			if (!(t = va_arg(argp, char *)))
				t = "(null)";
			if (prec >= 0) {
				/*
				 * can't use strlen; can only look for the
				 * NUL in the first `prec' characters, and
				 * strlen() will go further.
				 */
				char *p, *memchr();

				if (p = memchr(t, 0, prec)) {
					size = p - t;
					if (size > prec)
						size = prec;
				} else
					size = prec;
			} else
				size = strlen(t);
			sign = '\0';
			goto pforw;
		case 'U':
			flags |= LONGINT;
			/*FALLTHROUGH*/
		case 'u':
			ARG();
			base = 10;
			goto nosign;
		case 'X':
			digs = "0123456789ABCDEF";
			/* FALLTHROUGH */
		case 'x':
			ARG();
			base = 16;
			/* leading 0x/X only if non-zero */
			if (flags & ALT && _ulong != 0)
				flags |= HEXPREFIX;

			/* unsigned conversions */
nosign:			sign = '\0';
			/*
			 * ``... diouXx conversions ... if a precision is
			 * specified, the 0 flag will be ignored.''
			 *	-- ANSI X3J11
			 */
number:			if ((dprec = prec) >= 0)
				flags &= ~ZEROPAD;

			/*
			 * ``The result of converting a zero value with an
			 * explicit precision of zero is no characters.''
			 *	-- ANSI X3J11
			 */
			t = buf + BUF;
			if (_ulong != 0 || prec != 0) {
				do {
					*--t = digs[_ulong % base];
					_ulong /= base;
				} while (_ulong);
				digs = "0123456789abcdef";
				if (flags & ALT && base == 8 && *t != '0')
					*--t = '0'; /* octal leading 0 */
			}
			size = buf + BUF - t;

pforw:
			/*
			 * All reasonable formats wind up here.  At this point,
			 * `t' points to a string which (if not flags&LADJUST)
			 * should be padded out to `width' places.  If
			 * flags&ZEROPAD, it should first be prefixed by any
			 * sign or other prefix; otherwise, it should be blank
			 * padded before the prefix is emitted.  After any
			 * left-hand padding and prefixing, emit zeroes
			 * required by a decimal [diouxX] precision, then print
			 * the string proper, then emit zeroes required by any
			 * leftover floating precision; finally, if LADJUST,
			 * pad with blanks.
			 */

			/*
			 * compute actual size, so we know how much to pad
			 * fieldsz excludes decimal prec; realsz includes it
			 */
			fieldsz = size + fpprec;
			if (sign)
				fieldsz++;
			if (flags & HEXPREFIX)
				fieldsz += 2;
			realsz = dprec > fieldsz ? dprec : fieldsz;

			/* right-adjusting blank padding */
			if ((flags & (LADJUST|ZEROPAD)) == 0 && width)
				for (n = realsz; n < width; n++)
					PUTC(' ');
			/* prefix */
			if (sign)
				PUTC(sign);
			if (flags & HEXPREFIX) {
				PUTC('0');
				PUTC((char)*fmt);
			}
			/* right-adjusting zero padding */
			if ((flags & (LADJUST|ZEROPAD)) == ZEROPAD)
				for (n = realsz; n < width; n++)
					PUTC('0');
			/* leading zeroes from decimal precision */
			for (n = fieldsz; n < dprec; n++)
				PUTC('0');

			/* the string or number proper */
			n -= fieldsz-(size + fpprec);
			while (--n >= 0)
				PUTC(*t++);
			/* trailing f.p. zeroes */
			while (--fpprec >= 0)
				PUTC('0');
			/* left-adjusting padding (always blank) */
			if (flags & LADJUST)
				for (n = realsz; n < width; n++)
					PUTC(' ');
			/* finally, adjust cnt */
			cnt += width > realsz ? width : realsz;
			break;
		case '\0':	/* "%?" prints ?, unless ? is NULL */
			return (cnt);
		default:
			PUTC((char)*fmt);
			cnt++;
		}
	}
	/* NOTREACHED */
}

STATIC
cvt(number, prec, flags, signp, fmtch, startp, endp)
	double number;
	register int prec;
	int flags;
	unsigned char fmtch;
	char *signp, *startp, *endp;
{
	register char *p, *t;
	register double fract;
	int dotrim, expcnt, gformat;
	double integer, tmp;


	/*
	 * Check infinity and nan
	 */

	if (is_infinity(number)) {
		bcopy(INFINITY_STRING, startp, sizeof (INFINITY_STRING));
		return(sizeof (INFINITY_STRING)-1);
	}

	if (is_nan(number)) {
		bcopy(NAN_STRING, startp, sizeof (NAN_STRING));
		return(sizeof (NAN_STRING)-1);
	}

	dotrim = expcnt = gformat = 0;
	fract = modf(number, &integer);


	/* get an extra slot for rounding. */
	t = ++startp;

	/*
	 * get integer portion of number; put into the end of the buffer; the
	 * .01 is added for modf(356.0 / 10, &integer) returning .59999999...
	 */
	for (p = endp - 1; integer; ++expcnt) {
		tmp = modf(integer / 10, &integer);
		*p-- = tochar((int)((tmp + .01) * 10));
	}

	switch(fmtch) {
	case 'f':
		/* reverse integer into beginning of buffer */
		if (expcnt)
			for (; ++p < endp; *t++ = *p);
		else
			*t++ = '0';
		/*
		 * if precision required or alternate flag set, add in a
		 * decimal point.
		 */
		if (prec || flags&ALT)
			*t++ = '.';
		/* if requires more precision and some fraction left */
		if (fract) {
			if (prec)
				do {
					fract = modf(fract * 10, &tmp);
					*t++ = tochar((int)tmp);
				} while (--prec && fract);
			if (fract)
				startp = round(fract, (int *)0L, startp,
				    t - 1, (char)0, signp);
		}
		for (; prec--; *t++ = '0');
		break;
	case 'e':
	case 'E':
eformat:	if (expcnt) {
			*t++ = *++p;
			if (prec || flags&ALT)
				*t++ = '.';
			/* if requires more precision and some integer left */
			for (; prec && ++p < endp; --prec)
				*t++ = *p;
			/*
			 * if done precision and more of the integer component,
			 * round using it; adjust fract so we don't re-round
			 * later.
			 */
			if (!prec && ++p < endp) {
				fract = 0;
				startp = round((double)0, &expcnt, startp,
				    t - 1, *p, signp);
			}
			/* adjust expcnt for digit in front of decimal */
			--expcnt;
		}
		/* until first fractional digit, decrement exponent */
		else if (fract) {
			/* adjust expcnt for digit in front of decimal */
			for (expcnt = -1;; --expcnt) {
				fract = modf(fract * 10, &tmp);
				if (tmp)
					break;
			}
			*t++ = tochar((int)tmp);
			if (prec || flags&ALT)
				*t++ = '.';
		}
		else {
			*t++ = '0';
			if (prec || flags&ALT)
				*t++ = '.';
		}
		/* if requires more precision and some fraction left */
		if (fract) {
			if (prec)
				do {
					fract = modf(fract * 10, &tmp);
					*t++ = tochar((int)tmp);
				} while (--prec && fract);
			if (fract)
				startp = round(fract, &expcnt, startp,
				    t - 1, (char)0, signp);
		}
		/* if requires more precision */
		for (; prec--; *t++ = '0');

		/* unless alternate flag, trim any g/G format trailing 0's */
		if (gformat && !(flags&ALT)) {
			while (t > startp && *--t == '0');
			if (*t == '.')
				--t;
			++t;
		}
		t = exponent(t, expcnt, fmtch);
		break;
	case 'g':
	case 'G':
		/* a precision of 0 is treated as a precision of 1. */
		if (!prec)
			++prec;
		/*
		 * ``The style used depends on the value converted; style e
		 * will be used only if the exponent resulting from the
		 * conversion is less than -4 or greater than the precision.''
		 *	-- ANSI X3J11
		 */
		if (expcnt > prec || !expcnt && fract && fract < .0001) {
			/*
			 * g/G format counts "significant digits, not digits of
			 * precision; for the e/E format, this just causes an
			 * off-by-one problem, i.e. g/G considers the digit
			 * before the decimal point significant and e/E doesn't
			 * count it as precision.
			 */
			--prec;
			fmtch -= 2;		/* G->E, g->e */
			gformat = 1;
			goto eformat;
		}
		/*
		 * reverse integer into beginning of buffer,
		 * note, decrement precision
		 */
		if (expcnt)
			for (; ++p < endp; *t++ = *p, --prec);
		else
			*t++ = '0';
		/*
		 * if precision required or alternate flag set, add in a
		 * decimal point.  If no digits yet, add in leading 0.
		 */
		if (prec || flags&ALT) {
			dotrim = 1;
			*t++ = '.';
		}
		else
			dotrim = 0;
		/* if requires more precision and some fraction left */
		if (fract) {
			if (prec) {
				do {
					fract = modf(fract * 10, &tmp);
					*t++ = tochar((int)tmp);
				} while(!tmp);
				while (--prec && fract) {
					fract = modf(fract * 10, &tmp);
					*t++ = tochar((int)tmp);
				}
			}
			if (fract)
				startp = round(fract, (int *)0L, startp,
				    t - 1, (char)0, signp);
		}
		/* alternate format, adds 0's for precision, else trim 0's */
		if (flags&ALT)
			for (; prec--; *t++ = '0');
		else if (dotrim) {
			while (t > startp && *--t == '0');
			if (*t != '.')
				++t;
		}
	}
	return(t - startp);
}

STATIC char *
round(fract, exp, start, end, ch, signp)
	double fract;
	int *exp;
	register char *start, *end;
	char ch, *signp;
{
	double integer, tmp;

	if (fract)
		(void)modf(fract * 10, &tmp);
	else
		tmp = todigit(ch);
	if (tmp > 4)
		for (;; --end) {
			if (*end == '.')
				--end;
			if (++*end <= '9')
				break;
			*end = '0';
			if (end == start) {
				if (exp) {	/* e/E; increment exponent */
					*end = '1';
					++*exp;
				}
				else {		/* f; add extra digit */
					*--end = '1';
					--start;
				}
				break;
			}
		}
	/* ``"%.3f", (double)-0.0004'' gives you a negative 0. */
	else if (*signp == '-')
		for (;; --end) {
			if (*end == '.')
				--end;
			if (*end != '0')
				break;
			if (end == start)
				*signp = 0;
		}
	return(start);
}

STATIC char *
exponent(p, exp, fmtch)
	register char *p;
	register int exp;
	unsigned char fmtch;
{
	register char *t;
	char expbuf[MAXEXP];

	*p++ = fmtch;
	if (exp < 0) {
		exp = -exp;
		*p++ = '-';
	}
	else
		*p++ = '+';
	t = expbuf + MAXEXP;
	if (exp > 9) {
		do {
			*--t = tochar(exp % 10);
		} while ((exp /= 10) > 9);
		*--t = tochar(exp);
		for (; t < expbuf + MAXEXP; *p++ = *t++);
	}
	else {
		*p++ = '0';
		*p++ = tochar(exp);
	}
	return(p);
}

STATIC void
savechar(char **arg, int c)
{
	*(*arg)++ = c;
}

do_sprintf(char *s, char *fmt, va_list args)
{
	doprnt(fmt, args, 0, (void (*)()) savechar, (char *) &s);
	*s = 0;
}
