#pragma once 

#undef _CONCAT
#undef _EXPAND

//////////////////////////////////////
/////      IMPLEMENTATION        /////
//////////////////////////////////////
#define _PRIMITIVE_CONCAT(A, B) A ## B
#define _CONCAT(A, B, ...) _PRIMITIVE_CONCAT(A, B)

// The behavior of the preprocessor of MSVC is DIFFERENT from others, which will expand `__VA_ARGS__` after macro substitution, 
// so we have to force the expansion of these macro to get expected behavior. And Why didn't they just fix it?
#define _EXPAND(x) x

#define _GET_NTH_ARG(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, N, ...) N
#define _GET_NTH_OVERRIDE(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, M, ...) M
#define _GET_ARG_COUNT(...)                 _EXPAND(_GET_NTH_ARG(__VA_OPT__(__VA_ARGS__,) \
    20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0))
#define _GET_ARG_COUNT_MINUS_ONE(...)       _EXPAND(_GET_NTH_ARG(__VA_OPT__(__VA_ARGS__,) \
    19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0, 0))

#define _GET_ARG(N, ...) _CONCAT(_GET_ARG_, N)(__VA_ARGS__)
#define _GET_ARG_0(...)
#define _GET_ARG_1(_1, ...)                                                                                             _1
#define _GET_ARG_2(_1, _2, ...)                                                                                         _2
#define _GET_ARG_3(_1, _2, _3, ...)                                                                                     _3
#define _GET_ARG_4(_1, _2, _3, _4, ...)                                                                                 _4
#define _GET_ARG_5(_1, _2, _3, _4, _5, ...)                                                                             _5
#define _GET_ARG_6(_1, _2, _3, _4, _5, _6, ...)                                                                         _6
#define _GET_ARG_7(_1, _2, _3, _4, _5, _6,  _7, ...)                                                                    _7
#define _GET_ARG_8(_1, _2, _3, _4, _5, _6,  _7, _8, ...)                                                                _8
#define _GET_ARG_9(_1, _2, _3, _4, _5, _6,  _7, _8, _9, ...)                                                            _9
#define _GET_ARG_10(_1, _2, _3, _4, _5, _6,  _7, _8, _9, _10, ...)                                                      _10
#define _GET_ARG_11(_1, _2, _3, _4, _5, _6,  _7, _8, _9, _10, _11, ...)                                                 _11
#define _GET_ARG_12(_1, _2, _3, _4, _5, _6,  _7, _8, _9, _10, _11, _12, ...)                                            _12
#define _GET_ARG_13(_1, _2, _3, _4, _5, _6,  _7, _8, _9, _10, _11, _12, _13, ...)                                       _13
#define _GET_ARG_14(_1, _2, _3, _4, _5, _6,  _7, _8, _9, _10, _11, _12, _13, _14, ...)                                  _14
#define _GET_ARG_15(_1, _2, _3, _4, _5, _6,  _7, _8, _9, _10, _11, _12, _13, _14, _15, ...)                             _15
#define _GET_ARG_16(_1, _2, _3, _4, _5, _6,  _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, ...)                        _16
#define _GET_ARG_17(_1, _2, _3, _4, _5, _6,  _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, ...)                   _17
#define _GET_ARG_18(_1, _2, _3, _4, _5, _6,  _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, ...)              _18
#define _GET_ARG_19(_1, _2, _3, _4, _5, _6,  _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, ...)         _19
#define _GET_ARG_20(_1, _2, _3, _4, _5, _6,  _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, ...)    _20
#define _GET_ARG_LAST(...) _GET_ARG(_GET_ARG_COUNT(__VA_ARGS__), __VA_ARGS__)

#define _GET_PREVIOUS_ARG(N, ...) _CONCAT(_GET_PREVIOUS_ARG_, N)(__VA_ARGS__)
#define _GET_PREVIOUS_ARG_0(...)
#define _GET_PREVIOUS_ARG_1(_1, ...)                                                                                            _1
#define _GET_PREVIOUS_ARG_2(_1, _2, ...)                                                                                        _1, _2
#define _GET_PREVIOUS_ARG_3(_1, _2, _3, ...)                                                                                    _1, _2, _3 
#define _GET_PREVIOUS_ARG_4(_1, _2, _3, _4, ...)                                                                                _1, _2, _3, _4 
#define _GET_PREVIOUS_ARG_5(_1, _2, _3, _4, _5, ...)                                                                            _1, _2, _3, _4, _5 
#define _GET_PREVIOUS_ARG_6(_1, _2, _3, _4, _5, _6, ...)                                                                        _1, _2, _3, _4, _5, _6 
#define _GET_PREVIOUS_ARG_7(_1, _2, _3, _4, _5, _6,  _7, ...)                                                                   _1, _2, _3, _4, _5, _6, _7 
#define _GET_PREVIOUS_ARG_8(_1, _2, _3, _4, _5, _6,  _7, _8, ...)                                                               _1, _2, _3, _4, _5, _6, _7, _8 
#define _GET_PREVIOUS_ARG_9(_1, _2, _3, _4, _5, _6,  _7, _8, _9, ...)                                                           _1, _2, _3, _4, _5, _6, _7, _8, _9 
#define _GET_PREVIOUS_ARG_10(_1, _2, _3, _4, _5, _6,  _7, _8, _9, _10, ...)                                                     _1, _2, _3, _4, _5, _6, _7, _8, _9, _10 
#define _GET_PREVIOUS_ARG_11(_1, _2, _3, _4, _5, _6,  _7, _8, _9, _10, _11, ...)                                                _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11
#define _GET_PREVIOUS_ARG_12(_1, _2, _3, _4, _5, _6,  _7, _8, _9, _10, _11, _12, ...)                                           _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12
#define _GET_PREVIOUS_ARG_13(_1, _2, _3, _4, _5, _6,  _7, _8, _9, _10, _11, _12, _13, ...)                                      _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13 
#define _GET_PREVIOUS_ARG_14(_1, _2, _3, _4, _5, _6,  _7, _8, _9, _10, _11, _12, _13, _14, ...)                                 _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 
#define _GET_PREVIOUS_ARG_15(_1, _2, _3, _4, _5, _6,  _7, _8, _9, _10, _11, _12, _13, _14, _15, ...)                            _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, 
#define _GET_PREVIOUS_ARG_16(_1, _2, _3, _4, _5, _6,  _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, ...)                       _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, 
#define _GET_PREVIOUS_ARG_17(_1, _2, _3, _4, _5, _6,  _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, ...)                  _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17,  
#define _GET_PREVIOUS_ARG_18(_1, _2, _3, _4, _5, _6,  _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, ...)             _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18,   
#define _GET_PREVIOUS_ARG_19(_1, _2, _3, _4, _5, _6,  _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, ...)        _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19,    
#define _GET_PREVIOUS_ARG_20(_1, _2, _3, _4, _5, _6,  _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, ...)   _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20
#define _GET_ARG_EXCEPT_LAST(...) _GET_PREVIOUS_ARG(_GET_ARG_COUNT_MINUS_ONE(__VA_ARGS__), __VA_ARGS__)

#define _APPLY_TWO_PARAM_0(M, A, ...)                                   A
#define _APPLY_TWO_PARAM_1(M, A, B, ...)                                M(A, B)
#define _APPLY_TWO_PARAM_2(M, ...)                                      M(_APPLY_TWO_PARAM_1(M, _GET_ARG_EXCEPT_LAST(__VA_ARGS__), ,), _GET_ARG_LAST(__VA_ARGS__))
#define _APPLY_TWO_PARAM_3(M, ...)                                      M(_APPLY_TWO_PARAM_2(M, _GET_ARG_EXCEPT_LAST(__VA_ARGS__)), _GET_ARG_LAST(__VA_ARGS__))
#define _APPLY_TWO_PARAM_4(M, ...)                                      M(_APPLY_TWO_PARAM_3(M, _GET_ARG_EXCEPT_LAST(__VA_ARGS__)), _GET_ARG_LAST(__VA_ARGS__))
#define _APPLY_TWO_PARAM_5(M, ...)                                      M(_APPLY_TWO_PARAM_4(M, _GET_ARG_EXCEPT_LAST(__VA_ARGS__)), _GET_ARG_LAST(__VA_ARGS__))
#define _APPLY_TWO_PARAM_6(M, ...)                                      M(_APPLY_TWO_PARAM_5(M, _GET_ARG_EXCEPT_LAST(__VA_ARGS__)), _GET_ARG_LAST(__VA_ARGS__))
#define _APPLY_TWO_PARAM_7(M, ...)                                      M(_APPLY_TWO_PARAM_6(M, _GET_ARG_EXCEPT_LAST(__VA_ARGS__)), _GET_ARG_LAST(__VA_ARGS__))
#define _APPLY_TWO_PARAM_8(M, ...)                                      M(_APPLY_TWO_PARAM_7(M, _GET_ARG_EXCEPT_LAST(__VA_ARGS__)), _GET_ARG_LAST(__VA_ARGS__))
#define _APPLY_TWO_PARAM_9(M, ...)                                      M(_APPLY_TWO_PARAM_8(M, _GET_ARG_EXCEPT_LAST(__VA_ARGS__)), _GET_ARG_LAST(__VA_ARGS__))
#define _APPLY_TWO_PARAM_10(M, ...)                                     M(_APPLY_TWO_PARAM_9(M, _GET_ARG_EXCEPT_LAST(__VA_ARGS__)), _GET_ARG_LAST(__VA_ARGS__))
#define _APPLY_TWO_PARAM_11(M, ...)                                     M(_APPLY_TWO_PARAM_10(M, _GET_ARG_EXCEPT_LAST(__VA_ARGS__)), _GET_ARG_LAST(__VA_ARGS__))
#define _APPLY_TWO_PARAM_12(M, ...)                                     M(_APPLY_TWO_PARAM_11(M, _GET_ARG_EXCEPT_LAST(__VA_ARGS__)), _GET_ARG_LAST(__VA_ARGS__))
#define _APPLY_TWO_PARAM_13(M, ...)                                     M(_APPLY_TWO_PARAM_12(M, _GET_ARG_EXCEPT_LAST(__VA_ARGS__)), _GET_ARG_LAST(__VA_ARGS__))
#define _APPLY_TWO_PARAM_14(M, ...)                                     M(_APPLY_TWO_PARAM_13(M, _GET_ARG_EXCEPT_LAST(__VA_ARGS__)), _GET_ARG_LAST(__VA_ARGS__))
#define _APPLY_TWO_PARAM_15(M, ...)                                     M(_APPLY_TWO_PARAM_14(M, _GET_ARG_EXCEPT_LAST(__VA_ARGS__)), _GET_ARG_LAST(__VA_ARGS__))
#define _APPLY_TWO_PARAM_16(M, ...)                                     M(_APPLY_TWO_PARAM_15(M, _GET_ARG_EXCEPT_LAST(__VA_ARGS__)), _GET_ARG_LAST(__VA_ARGS__))
#define _APPLY_TWO_PARAM_17(M, ...)                                     M(_APPLY_TWO_PARAM_16(M, _GET_ARG_EXCEPT_LAST(__VA_ARGS__)), _GET_ARG_LAST(__VA_ARGS__))
#define _APPLY_TWO_PARAM_18(M, ...)                                     M(_APPLY_TWO_PARAM_17(M, _GET_ARG_EXCEPT_LAST(__VA_ARGS__)), _GET_ARG_LAST(__VA_ARGS__))
#define _APPLY_TWO_PARAM_19(M, ...)                                     M(_APPLY_TWO_PARAM_18(M, _GET_ARG_EXCEPT_LAST(__VA_ARGS__)), _GET_ARG_LAST(__VA_ARGS__))

#define _SELF_RECURSIVE_CALL(M, ...) _GET_NTH_OVERRIDE(ignore, ##__VA_ARGS__, \
    _APPLY_TWO_PARAM_19, \
    _APPLY_TWO_PARAM_18, \
    _APPLY_TWO_PARAM_17, \
    _APPLY_TWO_PARAM_16, \
    _APPLY_TWO_PARAM_15, \
    _APPLY_TWO_PARAM_14, \
    _APPLY_TWO_PARAM_13, \
    _APPLY_TWO_PARAM_12, \
    _APPLY_TWO_PARAM_11, \
    _APPLY_TWO_PARAM_10, \
    _APPLY_TWO_PARAM_9,  \
    _APPLY_TWO_PARAM_8,  \
    _APPLY_TWO_PARAM_7,  \
    _APPLY_TWO_PARAM_6,  \
    _APPLY_TWO_PARAM_5,  \
    _APPLY_TWO_PARAM_4,  \
    _APPLY_TWO_PARAM_3,  \
    _APPLY_TWO_PARAM_2,  \
    _APPLY_TWO_PARAM_1,  \
    _APPLY_TWO_PARAM_0) (M, __VA_ARGS__)
//////////////////////////////////////
/////      IMPLEMENTATION        /////
//////////////////////////////////////


//////////////////////////////////////
/////            API             /////
//////////////////////////////////////
#define CONCAT(...) _SELF_RECURSIVE_CALL(_CONCAT, __VA_ARGS__)
//////////////////////////////////////
/////            API             /////
//////////////////////////////////////