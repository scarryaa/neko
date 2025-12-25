#ifndef QT_TYPES_FWD_H
#define QT_TYPES_FWD_H

#include <QtGlobal>

#define QT_FWD_APPLY_1(M, A) M(A)
#define QT_FWD_APPLY_2(M, A, ...) M(A) QT_FWD_APPLY_1(M, __VA_ARGS__)
#define QT_FWD_APPLY_3(M, A, ...) M(A) QT_FWD_APPLY_2(M, __VA_ARGS__)
#define QT_FWD_APPLY_4(M, A, ...) M(A) QT_FWD_APPLY_3(M, __VA_ARGS__)
#define QT_FWD_APPLY_5(M, A, ...) M(A) QT_FWD_APPLY_4(M, __VA_ARGS__)
#define QT_FWD_APPLY_6(M, A, ...) M(A) QT_FWD_APPLY_5(M, __VA_ARGS__)
#define QT_FWD_APPLY_7(M, A, ...) M(A) QT_FWD_APPLY_6(M, __VA_ARGS__)
#define QT_FWD_APPLY_8(M, A, ...) M(A) QT_FWD_APPLY_7(M, __VA_ARGS__)
#define QT_FWD_APPLY_9(M, A, ...) M(A) QT_FWD_APPLY_8(M, __VA_ARGS__)
#define QT_FWD_APPLY_10(M, A, ...) M(A) QT_FWD_APPLY_9(M, __VA_ARGS__)
#define QT_FWD_APPLY_11(M, A, ...) M(A) QT_FWD_APPLY_10(M, __VA_ARGS__)
#define QT_FWD_APPLY_12(M, A, ...) M(A) QT_FWD_APPLY_11(M, __VA_ARGS__)
#define QT_FWD_APPLY_13(M, A, ...) M(A) QT_FWD_APPLY_12(M, __VA_ARGS__)
#define QT_FWD_APPLY_14(M, A, ...) M(A) QT_FWD_APPLY_13(M, __VA_ARGS__)
#define QT_FWD_APPLY_15(M, A, ...) M(A) QT_FWD_APPLY_14(M, __VA_ARGS__)

#define QT_FWD_GET_APPLY(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12,    \
                         _13, _14, NAME, ...)                                  \
  NAME
#define QT_FWD_FOR_EACH(M, ...)                                                \
  QT_FWD_GET_APPLY(                                                            \
      __VA_ARGS__, QT_FWD_APPLY_14, QT_FWD_APPLY_13, QT_FWD_APPLY_12,          \
      QT_FWD_APPLY_11, QT_FWD_APPLY_10, QT_FWD_APPLY_9, QT_FWD_APPLY_8,        \
      QT_FWD_APPLY_7, QT_FWD_APPLY_6, QT_FWD_APPLY_5, QT_FWD_APPLY_4,          \
      QT_FWD_APPLY_3, QT_FWD_APPLY_2, QT_FWD_APPLY_1)(M, __VA_ARGS__)

#define QT_FWD(...) QT_FWD_FOR_EACH(QT_FORWARD_DECLARE_CLASS, __VA_ARGS__)

#endif // QT_TYPES_FWD_H
