win32 {
    DEFINES += _CRT_NONSTDC_NO_DEPRECATE
    DEFINES += _CRT_SECURE_NO_WARNINGS
    INCLUDEPATH += "$$PWD/../iaito_win32/include"
    INCLUDEPATH += "$$PWD/../iaito_win32/radare2/include/libr"
    !contains(QMAKE_HOST.arch, x86_64) {
        LIBS += -L"$$PWD/../iaito_win32/radare2/lib32"
    } else {
        LIBS += -L"$$PWD/../iaito_win32/radare2/lib64"
    }
} else {
    # pseudo auto detection of libr paths, in the following order:
    # $HOME/bin/prefix/radare2 -> radare2: sys/user.sh
    # /usr/local -> radare2: sys/install.sh
    # /usr

    exists($$(HOME)/bin/prefix/radare2/include/libr) {
        RADARE2_INCLUDE_PATH = $$(HOME)/bin/prefix/radare2/include/libr
        RADARE2_LIB_PATH = $$(HOME)/bin/prefix/radare2/lib
        #message("found radare in $(HOME)/bin/prefix/radare2")
    }

    isEmpty(RADARE2_INCLUDE_PATH) {
        exists(/usr/local/include/libr) {
            RADARE2_INCLUDE_PATH = /usr/local/include/libr
            RADARE2_LIB_PATH = /usr/local/lib
            #message("found radare in /usr/local/")
        }
    }

    isEmpty(RADARE2_INCLUDE_PATH) {
        exists(/usr/include/libr) {
            RADARE2_INCLUDE_PATH = /usr/include/libr
            RADARE2_LIB_PATH = /usr/lib
            #message("found radare in /usr/")
        }
    }


    !isEmpty(RADARE2_INCLUDE_PATH) {
        INCLUDEPATH *= $$RADARE2_INCLUDE_PATH
        LIBS *= -L$$RADARE2_LIB_PATH
    } else {
        message("sorry could not find a radare2 lib")
    }
}

# are really all of this libs needed?
LIBS += \
    -lr_core \
    -lr_config \
    -lr_cons \
    -lr_io \
    -lr_util \
    -lr_flag \
    -lr_asm \
    -lr_debug \
    -lr_hash \
    -lr_bin \
    -lr_lang \
    -lr_io \
    -lr_anal \
    -lr_parse \
    -lr_bp \
    -lr_egg \
    -lr_reg \
    -lr_search \
    -lr_syscall \
    -lr_socket \
    -lr_fs \
    -lr_magic \
    -lr_crypto

