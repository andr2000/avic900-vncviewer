// q,w,e,r,t,y,u,i,o,p,a,s,d,f,g,h,j,k,l,z,x,c,v,b,n,m
int qwerty[] = {30,48,46,32,18,33,34,35,23,36,37,38,50,49,24,25,16,19,31,20,22,47,17,45,21,44};
//  ,!,",#,$,%,&,',(,),*,+,,,-,.,/
int spec1[] = {57,2,40,4,5,6,8,40,10,11,9,13,51,12,52,52};
int spec1sh[] = {0,1,1,1,1,1,1,0,1,1,1,1,0,0,0,1};
// :,;,<,=,>,?,@
int spec2[] = {39,39,227,13,228,53,215};
int spec2sh[] = {1,0,1,1,1,1,0};
// [,\,],^,_,`
int spec3[] = {26,43,27,7,12,399};
int spec3sh[] = {0,0,0,1,1,0};
// {,|,},~
int spec4[] = {26,43,27,215,14};
int spec4sh[] = {1,1,1,1,0};

int keycode(int c, bool &sh, bool &alt, bool real)
{
	if ('a' <= c && c <= 'z')
		return qwerty[c-'a'];
	if ('A' <= c && c <= 'Z')
	{
		sh = true;
		return qwerty[c-'A'];
	}
	if ('1' <= c && c <= '9')
		return c-'1'+2;
	if (c == '0')
		return 11;
	if (32 <= c && c <= 47)
	{
		sh = spec1sh[c-32];
		return spec1[c-32];
	}
	if (58 <= c && c <= 64)
	{
		sh = spec2sh[c-58];
		return spec2[c-58];
	}
	if (91 <= c && c <= 96)
	{
		sh = spec3sh[c-91];
		return spec3[c-91];
	}
	if (123 <= c && c <= 127)
	{
		sh = spec4sh[c-123];
		return spec4[c-123];
	}
	switch(c)
	{
		case 0xff08: return KEY_BACKSPACE;
		case 0xff09: return KEY_TAB;
		case 1: (alt) = true; return 34;// ctrl+a
		case 3: alt = true; return 46;// ctrl+c
		case 4: alt = true; return 32;// ctrl+d
		case 18: alt = true; return 31;// ctrl+r
		case 0xff0D: return KEY_ENTER;
		case 0xff1B: return KEY_BACK;
		case 0xFF51: return KEY_LEFT;
		case 0xFF53: return KEY_RIGHT;
		case 0xFF54: return KEY_DOWN;
		case 0xFF52: return KEY_UP;
		case 0xff50: return KEY_HOME;
		case 0xFFC8: return KEY_F11;
		case 0xFFC9: return KEY_F10;
		case 0xffc1: return KEY_F4;
		case 0xffff: return KEY_DELETE;
		case 0xff55: return 229;// PgUp -> menu
		case 0xffcf: return 127;// F2 -> search
		case 0xffe3: return 127;// left ctrl -> search
		case 0xff56: return 61;// PgUp -> call
		case 0xff57: return 107;// End -> endcall
		case 0xffc2: return 211;// F5 -> focus
		case 0xffc3: return 212;// F6 -> camera
		case 0xffc4: return 150;// F7 -> explorer
		case 0xffc5: return 155;// F8 -> envelope

		case 50081:
		case 225: alt = 1;
			if (real) return 48; //a with acute
			return 30; //a with acute -> a with ring above
		case 50049: 
		case 193:sh = 1; alt = 1; 
			if (real) return 48; //A with acute 
			return 30; //A with acute -> a with ring above
		case 50089:
		case 233: alt = 1; return 18; //e with acute
		case 50057: 
		case 201:sh = 1; alt = 1; return 18; //E with acute
		case 50093:
		case 237: alt = 1; 
			if (real) return 36; //i with acute 
			return 23; //i with acute -> i with grave
		case 50061:
		case 205: sh = 1; alt = 1; 
			if (real) return 36; //I with acute 
			return 23; //I with acute -> i with grave
		case 50099: 
		case 243:alt = 1; 
			if (real) return 16; //o with acute 
			return 24; //o with acute -> o with grave
		case 50067: 
		case 211:sh = 1; alt = 1; 
			if (real) return 16; //O with acute 
			return 24; //O with acute -> o with grave
		case 50102:
		case 246: alt = 1; return 25; //o with diaeresis
		case 50070:
		case 214: sh = 1; alt = 1; return 25; //O with diaeresis
		case 50577: 
		case 245:alt = 1; 
			if (real) return 19; //Hungarian o 
			return 25; //Hungarian o -> o with diaeresis
		case 50576:
		case 213: sh = 1; alt = 1; 
			if (real) return 19; //Hungarian O 
			return 25; //Hungarian O -> O with diaeresis
		case 50106:
		case 250: alt = 1; 
			if (real) return 17; //u with acute 
			return 22; //u with acute -> u with grave
		case 50074:
		case 218: sh = 1; alt = 1; 
			if (real) return 17; //U with acute 
			return 22; //U with acute -> u with grave
		case 50108:
		case 252: alt = 1; return 47; //u with diaeresis
		case 50076: 
		case 220:sh = 1; alt = 1; return 47; //U with diaeresis
		case 50609:
		case 251: alt = 1; 
			if (real) return 45; //Hungarian u 
			return 47; //Hungarian u -> u with diaeresis
		case 50608:
		case 219: sh = 1; alt = 1; 
			if (real) return 45; //Hungarian U 
			return 47; //Hungarian U -> U with diaeresis

	}
	return 0;
}
