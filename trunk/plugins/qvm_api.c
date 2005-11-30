/* An implementation of some 'standard' functions */

/* We use this with msvc too, because msvc's implementations of
   _snprintf and _vsnprint differ - besides, this way we can make 
   sure qvms handle all the printf stuff that dlls do*/
#include "plugin.h"

/*
this is a fairly basic implementation.
don't expect it to do much.
You can probably get a better version from somewhere.
*/
int vsnprintf(char *buffer, int maxlen, char *format, va_list vargs)
{
	int tokens=0;
	char *string;
	char tempbuffer[64];
	int _int;
	float _float;
	int i;
	int use0s;
	int precision;

	if (!maxlen)
		return 0;
maxlen--;

	while(*format)
	{
		switch(*format)
		{
		case '%':
			precision= 0;
			use0s=0;
retry:
			switch(*(++format))
			{
			case '0':
				if (!precision)
				{
					use0s=true;
					goto retry;
				}
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
				precision=precision*10+*format-'0';
				goto retry;
			case '%':	/*emit a %*/
				if (--maxlen < 0) 
					{*buffer++='\0';return tokens;}
				*buffer++ = *format;
				break;
			case 's':
				string = va_arg(vargs, char *);
				if (!string)
					string = "(null)";
				if (precision)
				{
					while (*string && precision--)
					{
						if (--maxlen < 0) 
							{*buffer++='\0';return tokens;}
						*buffer++ = *string++;
					}
				}
				else
				{
					while (*string)
					{
						if (--maxlen < 0) 
							{*buffer++='\0';return tokens;}
						*buffer++ = *string++;
					}
				}
				tokens++;
				break;
			case 'c':
				_int = va_arg(vargs, int);
				if (--maxlen < 0) 
					{*buffer++='\0';return tokens;}
				*buffer++ = _int;
				tokens++;
				break;
			case 'x':
				_int = va_arg(vargs, int);
				if (_int < 0)
				{
					if (--maxlen < 0) 
						{*buffer++='\0';return tokens;}
					*buffer++ = '-';
					_int *= -1;
				}
				i = sizeof(tempbuffer)-2;
				tempbuffer[sizeof(tempbuffer)-1] = '\0';
				while(_int)
				{
					tempbuffer[i] = _int%16 + '0';
					_int/=16;
					i--;
				}
				string = tempbuffer+i+1;

				if (!*string)
				{
					i=61;
					string = tempbuffer+i+1;
					string[0] = '0';
					string[1] = '\0';
				}

				precision -= 62-i;
				while (precision>0)
				{
					string--;
					if (use0s)
						*string = '0';
					else
						*string = ' ';
					precision--;
				}

				while (*string)
				{
					if (--maxlen < 0) 
						{*buffer++='\0';return tokens;}
					*buffer++ = *string++;
				}
				tokens++;
				break;
			case 'd':
			case 'u':
			case 'i':
				_int = va_arg(vargs, int);
				if (_int < 0)
				{
					if (--maxlen < 0) 
						{*buffer++='\0';return tokens;}
					*buffer++ = '-';
					_int *= -1;
				}
				i = sizeof(tempbuffer)-2;
				tempbuffer[sizeof(tempbuffer)-1] = '\0';
				while(_int)
				{
					tempbuffer[i] = _int%10 + '0';
					_int/=10;
					i--;
				}
				string = tempbuffer+i+1;

				if (!*string)
				{
					i=61;
					string = tempbuffer+i+1;
					string[0] = '0';
					string[1] = '\0';
				}

				precision -= 62-i;
				while (precision>0)
				{
					string--;
					if (use0s)
						*string = '0';
					else
						*string = ' ';
					precision--;
				}

				while (*string)
				{
					if (--maxlen < 0) 
						{*buffer++='\0';return tokens;}
					*buffer++ = *string++;
				}
				tokens++;
				break;
			case 'f':
				_float = (float)va_arg(vargs, double);

//integer part.
				_int = (int)_float;
				if (_int < 0)
				{
					if (--maxlen < 0) 
						{*buffer++='\0';return tokens;}
					*buffer++ = '-';
					_int *= -1;
				}
				i = sizeof(tempbuffer)-2;
				tempbuffer[sizeof(tempbuffer)-1] = '\0';
				if (!_int)
				{
					tempbuffer[i--] = '0';
				}
				else
				{
					while(_int)
					{
						tempbuffer[i--] = _int%10 + '0';
						_int/=10;
					}
				}
				string = tempbuffer+i+1;
				while (*string)
				{
					if (--maxlen < 0) 
						{*buffer++='\0';return tokens;}
					*buffer++ = *string++;
				}

				_int = sizeof(tempbuffer)-2-i;

//floating point part.
				_float -= (int)_float;
				i = 0;
				tempbuffer[i++] = '.';
				while(_float - (int)_float)
				{
					if (i + _int > 7)	//remove the excess presision.
						break;

					_float*=10;
					tempbuffer[i++] = (int)_float%10 + '0';
				}
				if (i == 1)	//no actual fractional part
				{
					tokens++;
					break;
				}

				//concatinate to our string
				tempbuffer[i] = '\0';
				string = tempbuffer;
				while (*string)
				{
					if (--maxlen < 0) 
						{*buffer++='\0';return tokens;}
					*buffer++ = *string++;
				}

				tokens++;
				break;
			default:
				string = "ERROR IN FORMAT";
				while (*string)
				{
					if (--maxlen < 0) 
						{*buffer++='\0';return tokens;}
					*buffer++ = *string++;
				}
				break;
			}
			break;
		default:
			if (--maxlen < 0) 
				{*buffer++='\0';return tokens;}
			*buffer++ = *format;
			break;
		}
		format++;
	}
	{*buffer++='\0';return tokens;}
}

int snprintf(char *buffer, int maxlen, char *format, ...)
{
	int p;
	va_list		argptr;
		
	va_start (argptr, format);
	p = vsnprintf (buffer, maxlen, format,argptr);
	va_end (argptr);

	return p;
}

#ifdef Q3_VM
int strlen(const char *s)
{
	int len = 0;
	while(*s++)
		len++;
	return len;
}

int strncmp (const char *s1, const char *s2, int count)
{
	while (1)
	{
		if (!count--)
			return 0;
		if (*s1 != *s2)
			return -1;		// strings not equal	
		if (!*s1)
			return 0;		// strings are equal
		s1++;
		s2++;
	}
	
	return -1;
}

int strnicmp(const char *s1, const char *s2, int count)
{
	char c1, c2;
	char ct;
	while(*s1)
	{
		if (!count--)
			return 0;
		c1 = *s1;
		c2 = *s2;
		if (c1 != c2)
		{
			if (c1 >= 'a' && c1 <= 'z') c1 = c1-'a' + 'A';
			if (c2 >= 'a' && c2 <= 'z') c2 = c2-'a' + 'A';
			if (c1 != c2)
				return c1<c2?-1:1;
		}
		s1++;
		s2++;
	}
	if (*s2)	//s2 was longer.
		return 1;
	return 0;
}

int strcmp(const char *s1, const char *s2)
{
	while(*s1)
	{
		if (*s1 != *s2)
			return *s1<*s2?-1:1;
		s1++;
		s2++;
	}
	if (*s2)	//s2 was longer.
		return 1;
	return 0;
}

int stricmp(const char *s1, const char *s2)
{
	char c1, c2;
	char ct;
	while(*s1)
	{
		c1 = *s1;
		c2 = *s2;
		if (c1 != c2)
		{
			if (c1 >= 'a' && c1 <= 'z') c1 = c1-'a' + 'A';
			if (c2 >= 'a' && c2 <= 'z') c2 = c2-'a' + 'A';
			if (c1 != c2)
				return c1<c2?-1:1;
		}
		s1++;
		s2++;
	}
	if (*s2)	//s2 was longer.
		return 1;
	return 0;
}

char *strstr(char *str, const char *sub)
{
	char *p;
	char *p2;
	int l = strlen(sub)-1;
	if (l < 0)
		return NULL;

	while(*str)
	{
		if (*str == *sub)
		{
			if (!strncmp (str+1, sub+1, l))
				return str;
		}	
		str++;
	}

	return NULL;
}
char *strchr(char *str, char sub)
{
	char *p;
	char *p2;

	while(*str)
	{
		if (*str == sub)
			return str;
		str++;
	}

	return NULL;
}

int atoi(char *str)
{
	int sign;
	int num = 0;
	int base = 10;

	while(*(unsigned char*)str < ' ' && *str)
		str++;

	if (*str == '-')
	{
		sign = -1;
		str++;
	}
	else sign = 1;

	if (str[0] == '0' && (str[1] == 'x' || str[1] == 'X'))
	{
		base = 16;
		str += 2;
	}

	while(1)
	{
		if (*str >= '0' && *str <= '9') 
			num = num*base + (*str - '0');
		else if (*str >= 'a' && *str <= 'a'+base-10)
			num = num*base + (*str - 'a')+10;
		else if (*str >= 'A' && *str <= 'A'+base-10)
			num = num*base + (*str - 'A')+10;
		else break;	//bad char
		str++;
	}	
	return num*sign;
}

float atof(char *str)
{
	int sign;
	float num = 0.0f;
	float unit = 1;

	while(*(unsigned char*)str < ' ' && *str)
		str++;

	if (*str == '-')
	{
		sign = -1;
		str++;
	}
	else sign = 1;

	while(1)
	{//each time we find a new digit, increase the value of the previous digets by a factor of ten, and add the new
		if (*str >= '0' && *str <= '9') 
			num = num*10 + (*str - '0');
		else break;	//bad char
		str++;
	}
	if (*str == '.')
	{	//each time we find a new digit, decrease the value of the following digits.
		while(1)
		{
			if (*str >= '0' && *str <= '9')
			{
				unit /= 10;
				num = num + (*str - '0')*unit;
			}
			else break;	//bad char
			str++;
		}
	}
	return num*sign;
}

void strcpy(char *d, const char *s)
{
	while (*s)
	{
		*d++ = *s++;
	}
	*d='\0';
}

static	long	randx = 1;
void srand(unsigned int x)
{
	randx = x;
}
int getseed(void)
{
	return randx;
}
int rand(void)
{
	return(((randx = randx*1103515245 + 12345)>>16) & 077777);
}


#endif

void strlcpy(char *d, const char *s, int n)
{
	int i;
	n--;
	if (n < 0)
		return;	//this could be an error

	for (i=0; *s; i++)
	{
		if (i == n)
			break;
		*d++ = *s++;
	}
	*d='\0';
}
