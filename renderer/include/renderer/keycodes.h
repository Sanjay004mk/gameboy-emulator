#pragma once

namespace rdr
{
	using KeyCode = uint16_t;
	using MouseCode = uint16_t;

#if defined(GEN_ENUM_STR_MAP)
#undef GEN_ENUM_STR_MAP
#define BEGIN_ENUM(Type) RDRAPI std::unordered_map<Type, const char*> Type##ToString = 
#define DEFINE_ELEM(Name, Value) { Value, #Name },
#define END_ENUM(Type) 
#else
#define BEGIN_ENUM(Type) enum : Type 
#define DEFINE_ELEM(Name, Value) Name = Value,
#define END_ENUM(Type)  extern RDRAPI std::unordered_map<Type, const char*> Type##ToString;
#endif

	namespace Key
	{
		BEGIN_ENUM(KeyCode)
		{
			// From glfw3.h
			DEFINE_ELEM(Space , 32)
			DEFINE_ELEM(Apostrophe , 39) /* ' */
			DEFINE_ELEM(Comma , 44) /* , */
			DEFINE_ELEM(Minus , 45) /* - */
			DEFINE_ELEM(Period , 46) /* . */
			DEFINE_ELEM(Slash , 47) /* / */

			DEFINE_ELEM(D0 , 48) /* 0 */
			DEFINE_ELEM(D1 , 49) /* 1 */
			DEFINE_ELEM(D2 , 50) /* 2 */
			DEFINE_ELEM(D3 , 51) /* 3 */
			DEFINE_ELEM(D4 , 52) /* 4 */
			DEFINE_ELEM(D5 , 53) /* 5 */
			DEFINE_ELEM(D6 , 54) /* 6 */
			DEFINE_ELEM(D7 , 55) /* 7 */
			DEFINE_ELEM(D8 , 56) /* 8 */
			DEFINE_ELEM(D9 , 57) /* 9 */

			DEFINE_ELEM(Semicolon , 59) /* ; */
			DEFINE_ELEM(Equal , 61) /* = */

			DEFINE_ELEM(A , 65)
			DEFINE_ELEM(B , 66)
			DEFINE_ELEM(C , 67)
			DEFINE_ELEM(D , 68)
			DEFINE_ELEM(E , 69)
			DEFINE_ELEM(F , 70)
			DEFINE_ELEM(G , 71)
			DEFINE_ELEM(H , 72)
			DEFINE_ELEM(I , 73)
			DEFINE_ELEM(J , 74)
			DEFINE_ELEM(K , 75)
			DEFINE_ELEM(L , 76)
			DEFINE_ELEM(M , 77)
			DEFINE_ELEM(N , 78)
			DEFINE_ELEM(O , 79)
			DEFINE_ELEM(P , 80)
			DEFINE_ELEM(Q , 81)
			DEFINE_ELEM(R , 82)
			DEFINE_ELEM(S , 83)
			DEFINE_ELEM(T , 84)
			DEFINE_ELEM(U , 85)
			DEFINE_ELEM(V , 86)
			DEFINE_ELEM(W , 87)
			DEFINE_ELEM(X , 88)
			DEFINE_ELEM(Y , 89)
			DEFINE_ELEM(Z , 90)

			DEFINE_ELEM(LeftBracket , 91)  /* [ */
			DEFINE_ELEM(Backslash , 92)  /* \ */
			DEFINE_ELEM(RightBracket , 93)  /* ] */
			DEFINE_ELEM(GraveAccent , 96)  /* ` */

			DEFINE_ELEM(World1 , 161) /* non-US #1 */
			DEFINE_ELEM(World2 , 162) /* non-US #2 */

			/* Function keys */
			DEFINE_ELEM(Escape , 256)
			DEFINE_ELEM(Enter , 257)
			DEFINE_ELEM(Tab , 258)
			DEFINE_ELEM(Backspace , 259)
			DEFINE_ELEM(Insert , 260)
			DEFINE_ELEM(Delete , 261)
			DEFINE_ELEM(Right , 262)
			DEFINE_ELEM(Left , 263)
			DEFINE_ELEM(Down , 264)
			DEFINE_ELEM(Up , 265)
			DEFINE_ELEM(PageUp , 266)
			DEFINE_ELEM(PageDown , 267)
			DEFINE_ELEM(Home , 268)
			DEFINE_ELEM(End , 269)
			DEFINE_ELEM(CapsLock , 280)
			DEFINE_ELEM(ScrollLock , 281)
			DEFINE_ELEM(NumLock , 282)
			DEFINE_ELEM(PrintScreen , 283)
			DEFINE_ELEM(Pause , 284)
			DEFINE_ELEM(F1 , 290)
			DEFINE_ELEM(F2 , 291)
			DEFINE_ELEM(F3 , 292)
			DEFINE_ELEM(F4 , 293)
			DEFINE_ELEM(F5 , 294)
			DEFINE_ELEM(F6 , 295)
			DEFINE_ELEM(F7 , 296)
			DEFINE_ELEM(F8 , 297)
			DEFINE_ELEM(F9 , 298)
			DEFINE_ELEM(F10 , 299)
			DEFINE_ELEM(F11 , 300)
			DEFINE_ELEM(F12 , 301)
			DEFINE_ELEM(F13 , 302)
			DEFINE_ELEM(F14 , 303)
			DEFINE_ELEM(F15 , 304)
			DEFINE_ELEM(F16 , 305)
			DEFINE_ELEM(F17 , 306)
			DEFINE_ELEM(F18 , 307)
			DEFINE_ELEM(F19 , 308)
			DEFINE_ELEM(F20 , 309)
			DEFINE_ELEM(F21 , 310)
			DEFINE_ELEM(F22 , 311)
			DEFINE_ELEM(F23 , 312)
			DEFINE_ELEM(F24 , 313)
			DEFINE_ELEM(F25 , 314)

			/* Keypad */
			DEFINE_ELEM(KP0 , 320)
			DEFINE_ELEM(KP1 , 321)
			DEFINE_ELEM(KP2 , 322)
			DEFINE_ELEM(KP3 , 323)
			DEFINE_ELEM(KP4 , 324)
			DEFINE_ELEM(KP5 , 325)
			DEFINE_ELEM(KP6 , 326)
			DEFINE_ELEM(KP7 , 327)
			DEFINE_ELEM(KP8 , 328)
			DEFINE_ELEM(KP9 , 329)
			DEFINE_ELEM(KPDecimal , 330)
			DEFINE_ELEM(KPDivide , 331)
			DEFINE_ELEM(KPMultiply , 332)
			DEFINE_ELEM(KPSubtract , 333)
			DEFINE_ELEM(KPAdd , 334)
			DEFINE_ELEM(KPEnter , 335)
			DEFINE_ELEM(KPEqual , 336)

			DEFINE_ELEM(LeftShift , 340)
			DEFINE_ELEM(LeftControl , 341)
			DEFINE_ELEM(LeftAlt , 342)
			DEFINE_ELEM(LeftSuper , 343)
			DEFINE_ELEM(RightShift , 344)
			DEFINE_ELEM(RightControl , 345)
			DEFINE_ELEM(RightAlt , 346)
			DEFINE_ELEM(RightSuper , 347)
			DEFINE_ELEM(Menu , 348)	
		};
		END_ENUM(KeyCode)
	}

	namespace Mouse
	{
		BEGIN_ENUM(MouseCode)
		{
			// From glfw3.h
			DEFINE_ELEM(Button0 , 0)
			DEFINE_ELEM(Button1 , 1)
			DEFINE_ELEM(Button2 , 2)
			DEFINE_ELEM(Button3 , 3)
			DEFINE_ELEM(Button4 , 4)
			DEFINE_ELEM(Button5 , 5)
			DEFINE_ELEM(Button6 , 6)
			DEFINE_ELEM(Button7 , 7)

			DEFINE_ELEM(ButtonLast , 7)
			DEFINE_ELEM(ButtonLeft , 0)
			DEFINE_ELEM(ButtonRight , 1)
			DEFINE_ELEM(ButtonMiddle , 2)	
		};
		END_ENUM(MouseCode);
	}
}

#undef BEGIN_ENUM
#undef DEFINE_ELEM
#undef END_ENUM