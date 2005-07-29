dnl
dnl AM_PATH_RAPTOR(MINIMUM-VERSION, [ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]])
dnl
AC_DEFUN([AM_PATH_RAPTOR],
[

AC_ARG_WITH(raptor-prefix,[  --with-raptor-prefix=PFX   Prefix where Raptor is installed (optional)],
            raptor_config_prefix="$withval", raptor_config_prefix="")
AC_ARG_WITH(raptor-exec-prefix,[  --with-raptor-exec-prefix=PFX  Exec prefix where Raptor is installed (optional)],
            raptor_config_exec_prefix="$withval", raptor_config_exec_prefix="")

  if test x$raptor_config_exec_prefix != x ; then
     raptor_config_args="$raptor_config_args --exec-prefix=$raptor_config_exec_prefix"
     if test x${RAPTOR_CONFIG+set} != xset ; then
        RAPTOR_CONFIG=$raptor_config_exec_prefix/bin/raptor-config
     fi
  fi
  if test x$raptor_config_prefix != x ; then
     raptor_config_args="$raptor_config_args --prefix=$raptor_config_prefix"
     if test x${RAPTOR_CONFIG+set} != xset ; then
        RAPTOR_CONFIG=$raptor_config_prefix/bin/raptor-config
     fi
  fi

  AC_PATH_PROG(RAPTOR_CONFIG, raptor-config, no)
  raptor_version_min=$1

  AC_MSG_CHECKING(for Raptor - version >= $raptor_version_min)
  no_raptor=""
  if test "$RAPTOR_CONFIG" = "no" ; then
    no_raptor=yes
  else
    RAPTOR_CFLAGS=`$RAPTOR_CONFIG --cflags`
    RAPTOR_LIBS=`$RAPTOR_CONFIG --libs`
    raptor_version=`$RAPTOR_CONFIG --version`

    raptor_major_version=`echo $raptor_version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\1/'`
    raptor_minor_version=`echo $raptor_version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\2/'`
    raptor_micro_version=`echo $raptor_version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\3/'`

    raptor_major_min=`echo $raptor_version_min | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\1/'`
    raptor_minor_min=`echo $raptor_version_min | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\2/'`
    raptor_micro_min=`echo $raptor_version_min | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\3/'`

    raptor_version_proper=`expr \
        $raptor_major_version \> $raptor_major_min \| \
        $raptor_major_version \= $raptor_major_min \& \
        $raptor_minor_version \> $raptor_minor_min \| \
        $raptor_major_version \= $raptor_major_min \& \
        $raptor_minor_version \= $raptor_minor_min \& \
        $raptor_micro_version \>= $raptor_micro_min `

    if test "$raptor_version_proper" = "1" ; then
      AC_MSG_RESULT([$raptor_major_version.$raptor_minor_version.$raptor_micro_version])
    else
      AC_MSG_RESULT(no)
      no_raptor=yes
    fi
  fi

  if test "x$no_raptor" = x ; then
     ifelse([$2], , :, [$2])     
  else
     RAPTOR_CFLAGS=""
     RAPTOR_LIBS=""
     ifelse([$3], , :, [$3])
  fi

  AC_SUBST(RAPTOR_CFLAGS)
  AC_SUBST(RAPTOR_LIBS)
])



