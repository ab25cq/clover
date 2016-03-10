#######################################################
# installed directories
#######################################################
prefix=/home/ab25cq
exec_prefix=${prefix}
bindir=${exec_prefix}/bin
datadir=${datarootdir}
mandir=${datarootdir}/man
libdir=${exec_prefix}/lib
sharedstatedir=${prefix}/com
sysconfdir=${prefix}/etc/clover
includedir=${prefix}/include/clover
datarootdir=${prefix}/share/clover
docdir=${datarootdir}/doc

##########################################################
# environmnet variables
##########################################################
CC=clang
INSTALL=/usr/bin/install -c
CFLAGS=-Isrc/ -I. -L . -I/home/ab25cq/include -L/home/ab25cq/lib -fPIC -DSYSCONFDIR="\"${sysconfdir}/\"" -DDOCDIR="\"${docdir}/\"" -DDATAROOTDIR="\"${datarootdir}/\"" -I/usr/local/include -L /usr/local/lib -g -DMDEBUG -Werror -Qunused-arguments
LIBS= -lutil -lonig -lpthread -lreadline -ldl -lm -lcursesw -lncursesw
OBJ=src/main.o src/utf_mb_str.o src/heap.o src/buffer.o src/klass.o src/vm.o src/debug.o src/xfunc.o src/interface.o src/type.o src/c_to_clover.o src/obj_clover.o src/obj_int.o src/obj_byte.o src/obj_short.o src/obj_uint.o src/obj_long.o src/obj_char.o src/obj_bytes.o src/obj_float.o src/obj_double.o src/obj_bool.o src/obj_pointer.o src/obj_null.o src/obj_class_object.o src/obj_user_object.o src/obj_string.o src/obj_string_buffer.o src/obj_parser.o src/obj_oniguruma_regex.o src/obj_array.o src/obj_hash.o src/obj_range.o src/obj_block.o src/obj_system.o src/obj_thread.o src/obj_mutex.o src/obj_file.o src/obj_file.o src/obj_type_object.o src/obj_void.o src/obj_anonymous.o src/obj_field.o src/obj_method.o src/obj_enum.o src/obj_wait_status.o src/obj_tm.o src/clover2llvm.o
DESTDIR=
SO_VERSION=1.0.0
LIBSONAME=libclover.so
LIBSO2NAME=libclover.so.1.0.0
OS=LINUX

##########################################################
# main
##########################################################
all: lib int.clo iclover cclover clover pclover psclover llclover $(ICLOVER) make_clover_home
#	rm -f install
	if test $(OS) = DARWIN; then ctags src/*.c > /dev/null 2>&1; else ctags -R; fi

make_clover_home:
	if test -x /usr/bin/mkdir; then /usr/bin/mkdir -p ~/.clover; else /bin/mkdir -p ~/.clover; fi
	if test -x /usr/bin/chmod; then /usr/bin/chmod 700 ~/.clover; else /bin/chmod 700 ~/.clover; fi
	if test -x /usr/bin/mkdir; then /usr/bin/mkdir -p ~/.clover/tmpfiles; else /bin/mkdir -p ~/.clover/tmpfiles; fi
	if test -x /usr/bin/chmod; then /usr/bin/chmod 700 ~/.clover/tmpfiles; else /bin/chmod 700 ~/.clover/tmpfiles; fi

int.clo: cclover cclover Fundamental.clc
	rm -f *.clm *.clo
	if test $(OS) = DARWIN; then DYLD_LIBRARY_PATH=.:$(DYLD_LIBRARY_PATH) ./cclover --no-load-fundamental-classes Fundamental.clc; else LD_LIBRARY_PATH=.:$(LD_LIBRARY_PATH) ./cclover --no-load-fundamental-classes Fundamental.clc; fi

clover: config.h src/main.c $(LIBSONAME)
	clang -o clover src/main.c $(CFLAGS:-static=) -lclover $(LIBS)

cclover: config.h src/compiler.c src/load_class.o src/parse.o src/node.o src/node_tree.o src/node_type.o src/compile.o src/klass_ctime.o src/vtable.o src/alias.o src/namespace.o src/module.o src/errmsg.o $(LIBSONAME)
	clang -o cclover src/compiler.c src/load_class.o src/parse.o src/node.o src/compile.o src/node_tree.o src/node_type.o src/klass_ctime.o src/vtable.o src/alias.o src/namespace.o src/module.o src/errmsg.o $(CFLAGS:-static=) -lclover $(LIBS)

llclover: src/llcompiler.o config.h Makefile $(LIBSONAME)
	clang++ -o llclover src/llcompiler.o -lclover $(LIBS) `llvm-config --cxxflags --ldflags --libs core executionengine interpreter analysis native bitwriter --system-libs` -Qunused-arguments $(CFLAGS)

src/llcompiler.o: src/llcompiler.c config.h Makefile $(LIBSONAME)
	clang -c src/llcompiler.c -o src/llcompiler.o `llvm-config --cflags` $(CFLAGS)

psclover: config.h src/parser.c src/load_class.o src/parse.o src/node.o src/node_tree.o src/node_type.o src/compile.o src/klass_ctime.o src/vtable.o src/alias.o src/namespace.o src/module.o src/errmsg.o $(LIBSONAME)
	clang -o psclover src/parser.c src/load_class.o src/parse.o src/node.o src/compile.o src/node_tree.o src/node_type.o src/klass_ctime.o src/vtable.o src/alias.o src/namespace.o src/module.o src/errmsg.o $(CFLAGS:-static=) -lclover $(LIBS)

pclover: config.h src/preprocessor.o $(LIBSONAME)
	clang -o pclover src/preprocessor.o $(CFLAGS:-static=) -lclover $(LIBS)

iclover: config.h src/interpreter.c $(LIBSONAME)
	clang -o iclover src/interpreter.c $(CFLAGS:-static=) -lclover $(LIBS)

lib: $(LIBSONAME)
#	rm -f install

lib-install:
	if [ -z "$(DESTDIR)" ]; then make lib-normal-install; else make lib-dest-install; fi

lib-normal-install:
	mkdir -p "$(libdir)"
	if [ $(LIBSONAME) = libclover.so ]; then if echo $(CFLAGS) | grep -q MDEBUG; then $(INSTALL) -m 755 libclover.so.$(SO_VERSION) "$(libdir)"; else $(INSTALL) -s -m 755 libclover.so.$(SO_VERSION) "$(libdir)"; fi; elif [ $(LIBSONAME) = libclover.dylib ]; then $(INSTALL) -m 755 libclover.$(SO_VERSION).dylib "$(libdir)"; fi
	if [ $(LIBSONAME) = libclover.so ]; then ln -s -f libclover.so.$(SO_VERSION) "$(libdir)"/libclover.so.1; elif [ $(LIBSONAME) = libclover.dylib ]; then ln -s -f libclover.$(SO_VERSION).dylib "$(libdir)"/libclover.1.dylib; fi
	if [ $(LIBSONAME) = libclover.so ]; then ln -s -f libclover.so.$(SO_VERSION) "$(libdir)"/libclover.so; elif [ $(LIBSONAME) = libclover.dylib ]; then ln -s -f libclover.$(SO_VERSION).dylib "$(libdir)"/libclover.dylib; fi

lib-dest-install:
	mkdir -p "$(DESTDIR)/$(libdir)"
	if [ "$(LIBSONAME)" = libclover.so ]; then if echo $(CFLAGS) | grep -q MDEBUG; then $(INSTALL) -m 755 libclover.so.$(SO_VERSION) "$(DESTDIR)/$(libdir)"; else $(INSTALL) -s -m 755 libclover.so.$(SO_VERSION) "$(DESTDIR)/$(libdir)"; fi; elif [ "$(LIBSONAME)" = libclover.dylib ]; then $(INSTALL) -m 755 libclover.$(SO_VERSION).dylib "$(DESTDIR)/$(libdir)"; fi
	if [ "$(LIBSONAME)" = libclover.so ]; then ln -s -f libclover.so.$(SO_VERSION) "$(DESTDIR)/$(libdir)"/libclover.so.1; elif [ "$(LIBSONAME)" = libclover.dylib ]; then ln -s -f libclover.$(SO_VERSION).dylib "$(DESTDIR)/$(libdir)"/libclover.1.dylib; fi
	if [ "$(LIBSONAME)" = libclover.so ]; then ln -s -f libclover.so.$(SO_VERSION) "$(DESTDIR)/$(libdir)"/libclover.so; elif [ "$(LIBSONAME)" = libclover.dylib ]; then ln -s -f libclover.$(SO_VERSION).dylib "$(DESTDIR)/$(libdir)"/libclover.dylib; fi

########################################################
# clover libraries
########################################################
libclover.so.$(SO_VERSION): $(OBJ)
	clang -shared -o libclover.so.$(SO_VERSION) $(OBJ) -lc $(LIBS) $(CFLAGS)

libclover.so: libclover.so.$(SO_VERSION)
	ln -s -f libclover.so.$(SO_VERSION) libclover.so.1
	ln -s -f libclover.so.$(SO_VERSION) libclover.so

########################################################
# clover libraries on Darwin
########################################################
libclover.$(SO_VERSION).dylib: $(OBJ)
	clang -dynamiclib -o libclover.$(SO_VERSION).dylib $(OBJ) -lc $(LIBS) $(CFLAGS)

libclover.dylib: libclover.$(SO_VERSION).dylib
	cp libclover.$(SO_VERSION).dylib libclover.1.dylib
	cp libclover.$(SO_VERSION).dylib libclover.dylib

#########################################################
# Object files
#########################################################
$(OBJ): src/*.h Makefile configure

#########################################################
# install
#########################################################
install: StandardLibrary.clc fundamental-install lib-install 
	./cclover StandardLibrary.clc
	if [ -z "$(DESTDIR)" ]; then make stdlib-install; else make stdlib-dest-install; fi
	./cclover Completion.clc
	if [ -z "$(DESTDIR)" ]; then make completion-install; else make completion-dest-install; fi

StandardLibrary.clc: StandardLibrary.clc.in
	./pclover StandardLibrary.clc.in

fundamental-install:
	if [ -z "$(DESTDIR)" ]; then make normal-install; else make dest-install; fi

stdlib-install:
	$(INSTALL) -m 644 mode_t.clo "$(datarootdir)"
	$(INSTALL) -m 644 dev_t.clo "$(datarootdir)"
	$(INSTALL) -m 644 permission.clo "$(datarootdir)"
	$(INSTALL) -m 644 uid_t.clo "$(datarootdir)"
	$(INSTALL) -m 644 gid_t.clo "$(datarootdir)"
	$(INSTALL) -m 644 off_t.clo "$(datarootdir)"
	$(INSTALL) -m 644 time_t.clo "$(datarootdir)"
	$(INSTALL) -m 644 pid_t.clo "$(datarootdir)"
	$(INSTALL) -m 644 tm.clo "$(datarootdir)"
	$(INSTALL) -m 644 AccessMode.clo "$(datarootdir)"
	$(INSTALL) -m 644 WaitOption.clo "$(datarootdir)"
	$(INSTALL) -m 644 utimbuf.clo "$(datarootdir)"
	$(INSTALL) -m 644 FnmatchFlags.clo "$(datarootdir)"
	$(INSTALL) -m 644 DIR.clo "$(datarootdir)"
	$(INSTALL) -m 644 dirent.clo "$(datarootdir)"
	$(INSTALL) -m 644 FileAccess.clo "$(datarootdir)"
	$(INSTALL) -m 644 FileLockOperation.clo "$(datarootdir)"
	$(INSTALL) -m 644 stat.clo "$(datarootdir)"
	$(INSTALL) -m 644 Path.clo "$(datarootdir)"
	$(INSTALL) -m 644 Directory.clo "$(datarootdir)"
	$(INSTALL) -m 644 FileMode.clo "$(datarootdir)"
	$(INSTALL) -m 644 FileKind.clo "$(datarootdir)"
	$(INSTALL) -m 644 WaitStatus.clo "$(datarootdir)"
	$(INSTALL) -m 644 System\#2.clo "$(datarootdir)"
	$(INSTALL) -m 644 String\#2.clo "$(datarootdir)"
	$(INSTALL) -m 644 uint\#2.clo "$(datarootdir)"
	$(INSTALL) -m 644 Clover\#2.clo "$(datarootdir)"
	$(INSTALL) -m 644 long\#2.clo "$(datarootdir)"
	$(INSTALL) -m 644 int\#2.clo "$(datarootdir)"
	$(INSTALL) -m 644 FileBase.clo "$(datarootdir)"
	$(INSTALL) -m 644 FileBase.clo "$(datarootdir)"
	$(INSTALL) -m 644 File.clo "$(datarootdir)"
	$(INSTALL) -m 644 Command.clo "$(datarootdir)"
	$(INSTALL) -m 644 termios.clo "$(datarootdir)"
	$(INSTALL) -m 644 cc_t.clo "$(datarootdir)"
	$(INSTALL) -m 644 tcflag_t.clo "$(datarootdir)"
	$(INSTALL) -m 644 TCSetAttrAction.clo "$(datarootdir)"
	$(INSTALL) -m 644 Signal.clo "$(datarootdir)"
	$(INSTALL) -m 644 Jobs.clo "$(datarootdir)"
	$(INSTALL) -m 644 Job.clo "$(datarootdir)"

stdlib-dest-install:
	$(INSTALL) -m 644 mode_t.clo "$(DESTDIR)/$(datarootdir)""
	$(INSTALL) -m 644 dev_t.clo "$(DESTDIR)/$(datarootdir)""
	$(INSTALL) -m 644 permission.clo "$(DESTDIR)/$(datarootdir)""
	$(INSTALL) -m 644 uid_t.clo "$(DESTDIR)/$(datarootdir)""
	$(INSTALL) -m 644 gid_t.clo "$(DESTDIR)/$(datarootdir)""
	$(INSTALL) -m 644 off_t.clo "$(DESTDIR)/$(datarootdir)""
	$(INSTALL) -m 644 time_t.clo "$(DESTDIR)/$(datarootdir)""
	$(INSTALL) -m 644 pid_t.clo "$(DESTDIR)/$(datarootdir)""
	$(INSTALL) -m 644 tm.clo "$(DESTDIR)/$(datarootdir)""
	$(INSTALL) -m 644 AccessMode.clo "$(DESTDIR)/$(datarootdir)""
	$(INSTALL) -m 644 WaitOption.clo "$(DESTDIR)/$(datarootdir)""
	$(INSTALL) -m 644 utimbuf.clo "$(DESTDIR)/$(datarootdir)""
	$(INSTALL) -m 644 FnmatchFlags.clo "$(DESTDIR)/$(datarootdir)""
	$(INSTALL) -m 644 DIR.clo "$(DESTDIR)/$(datarootdir)""
	$(INSTALL) -m 644 dirent.clo "$(DESTDIR)/$(datarootdir)""
	$(INSTALL) -m 644 FileAccess.clo "$(DESTDIR)/$(datarootdir)""
	$(INSTALL) -m 644 FileLockOperation.clo "$(DESTDIR)/$(datarootdir)""
	$(INSTALL) -m 644 stat.clo "$(DESTDIR)/$(datarootdir)""
	$(INSTALL) -m 644 Path.clo "$(DESTDIR)/$(datarootdir)""
	$(INSTALL) -m 644 Directory.clo "$(DESTDIR)/$(datarootdir)""
	$(INSTALL) -m 644 FileMode.clo "$(DESTDIR)/$(datarootdir)""
	$(INSTALL) -m 644 FileKind.clo "$(DESTDIR)/$(datarootdir)""
	$(INSTALL) -m 644 WaitStatus.clo "$(DESTDIR)/$(datarootdir)""
	$(INSTALL) -m 644 System\#2.clo "$(DESTDIR)/$(datarootdir)""
	$(INSTALL) -m 644 String\#2.clo "$(DESTDIR)/$(datarootdir)""
	$(INSTALL) -m 644 uint\#2.clo "$(DESTDIR)/$(datarootdir)""
	$(INSTALL) -m 644 Clover\#2.clo "$(DESTDIR)/$(datarootdir)""
	$(INSTALL) -m 644 long\#2.clo "$(DESTDIR)/$(datarootdir)""
	$(INSTALL) -m 644 int\#2.clo "$(DESTDIR)/$(datarootdir)""
	$(INSTALL) -m 644 FileBase.clo "$(DESTDIR)/$(datarootdir)""
	$(INSTALL) -m 644 FileBase.clo "$(DESTDIR)/$(datarootdir)""
	$(INSTALL) -m 644 File.clo "$(DESTDIR)/$(datarootdir)""
	$(INSTALL) -m 644 Command.clo "$(DESTDIR)/$(datarootdir)""
	$(INSTALL) -m 644 termios.clo "$(DESTDIR)/$(datarootdir)"
	$(INSTALL) -m 644 cc_t.clo "$(DESTDIR)/$(datarootdir)"
	$(INSTALL) -m 644 tcflag_t.clo "$(DESTDIR)/$(datarootdir)"
	$(INSTALL) -m 644 TCSetAttrAction.clo "$(DESTDIR)/$(datarootdir)"
	$(INSTALL) -m 644 Signal.clo "$(DESTDIR)/$(datarootdir)"
	$(INSTALL) -m 644 Jobs.clo "$(DESTDIR)/$(datarootdir)"
	$(INSTALL) -m 644 Job.clo "$(DESTDIR)/$(datarootdir)"

completion-install:
	$(INSTALL) -m 644 Command\#2.clo "$(datarootdir)"

completion-dest-install:
	$(INSTALL) -m 644 Command\#2.clo "$(DESTDIR)/$(datarootdir)"

normal-install:
	mkdir -p "$(bindir)"
	mkdir -p "$(sysconfdir)"
	mkdir -p "$(libdir)"
	mkdir -p "$(includedir)"
	mkdir -p "$(docdir)"
	mkdir -p "$(mandir)/man1"
	$(INSTALL) -m 644 src/clover.h "$(includedir)"
	$(INSTALL) -m 644 USAGE "$(docdir)"
	$(INSTALL) -m 644 README "$(docdir)"
	$(INSTALL) -m 644 CHANGELOG "$(docdir)"
	$(INSTALL) -m 644 man/man1/clover.1 "$(mandir)/man1"
	$(INSTALL) -m 644 Object.clo "$(datarootdir)"
	$(INSTALL) -m 644 int.clo "$(datarootdir)"
	$(INSTALL) -m 644 byte.clo "$(datarootdir)"
	$(INSTALL) -m 644 short.clo "$(datarootdir)"
	$(INSTALL) -m 644 uint.clo "$(datarootdir)"
	$(INSTALL) -m 644 long.clo "$(datarootdir)"
	$(INSTALL) -m 644 char.clo "$(datarootdir)"
	$(INSTALL) -m 644 float.clo "$(datarootdir)"
	$(INSTALL) -m 644 double.clo "$(datarootdir)"
	$(INSTALL) -m 644 bool.clo "$(datarootdir)"
	$(INSTALL) -m 644 pointer.clo "$(datarootdir)"
	$(INSTALL) -m 644 Encoding.clo "$(datarootdir)"
	$(INSTALL) -m 644 Regex.clo "$(datarootdir)"
	$(INSTALL) -m 644 OnigurumaRegex.clo "$(datarootdir)"
	$(INSTALL) -m 644 String.clo "$(datarootdir)"
	$(INSTALL) -m 644 StringBuffer.clo "$(datarootdir)"
	$(INSTALL) -m 644 Parser.clo "$(datarootdir)"
	$(INSTALL) -m 644 Bytes.clo "$(datarootdir)"
	$(INSTALL) -m 644 Range.clo "$(datarootdir)"
	$(INSTALL) -m 644 'Array$$1.clo' "$(datarootdir)"
	$(INSTALL) -m 644 'SortableArray$$1.clo' "$(datarootdir)"
	$(INSTALL) -m 644 'Hash$$2.clo' "$(datarootdir)"
	$(INSTALL) -m 644 'Tuple$$1.clo' "$(datarootdir)"
	$(INSTALL) -m 644 'Tuple$$2.clo' "$(datarootdir)"
	$(INSTALL) -m 644 'Tuple$$3.clo' "$(datarootdir)"
	$(INSTALL) -m 644 'Tuple$$4.clo' "$(datarootdir)"
	$(INSTALL) -m 644 'Tuple$$5.clo' "$(datarootdir)"
	$(INSTALL) -m 644 'Tuple$$6.clo' "$(datarootdir)"
	$(INSTALL) -m 644 'Tuple$$7.clo' "$(datarootdir)"
	$(INSTALL) -m 644 'Tuple$$8.clo' "$(datarootdir)"
	$(INSTALL) -m 644 Field.clo "$(datarootdir)"
	$(INSTALL) -m 644 Method.clo "$(datarootdir)"
	$(INSTALL) -m 644 Block.clo "$(datarootdir)"
	$(INSTALL) -m 644 GenericsParametor.clo "$(datarootdir)"
	$(INSTALL) -m 644 Class.clo "$(datarootdir)"
	$(INSTALL) -m 644 Type.clo "$(datarootdir)"
	$(INSTALL) -m 644 Enum.clo "$(datarootdir)"
	$(INSTALL) -m 644 Null.clo "$(datarootdir)"
	$(INSTALL) -m 644 anonymous.clo "$(datarootdir)"
	$(INSTALL) -m 644 void.clo "$(datarootdir)"
	$(INSTALL) -m 644 Thread.clo "$(datarootdir)"
	$(INSTALL) -m 644 Mutex.clo "$(datarootdir)"
	$(INSTALL) -m 644 Clover.clo "$(datarootdir)"
	$(INSTALL) -m 644 System.clo "$(datarootdir)"
	$(INSTALL) -m 644 Exception.clo "$(datarootdir)"
	$(INSTALL) -m 644 SystemException.clo "$(datarootdir)"
	$(INSTALL) -m 644 NullPointerException.clo "$(datarootdir)"
	$(INSTALL) -m 644 RangeException.clo "$(datarootdir)"
	$(INSTALL) -m 644 ConvertingStringCodeException.clo "$(datarootdir)"
	$(INSTALL) -m 644 ClassNotFoundException.clo "$(datarootdir)"
	$(INSTALL) -m 644 IOException.clo "$(datarootdir)"
	$(INSTALL) -m 644 OverflowException.clo "$(datarootdir)"
	$(INSTALL) -m 644 CantSolveGenericsType.clo "$(datarootdir)"
	$(INSTALL) -m 644 TypeError.clo "$(datarootdir)"
	$(INSTALL) -m 644 MethodMissingException.clo "$(datarootdir)"
	$(INSTALL) -m 644 DivisionByZeroException.clo "$(datarootdir)"
	$(INSTALL) -m 644 OverflowStackSizeException.clo "$(datarootdir)"
	$(INSTALL) -m 644 InvalidRegexException.clo "$(datarootdir)"
	$(INSTALL) -m 644 KeyNotFoundException.clo "$(datarootdir)"
	$(INSTALL) -m 644 KeyOverlappingException.clo "$(datarootdir)"
	$(INSTALL) -m 644 OutOfRangeOfStackException.clo "$(datarootdir)"
	$(INSTALL) -m 644 OutOfRangeOfFieldException.clo "$(datarootdir)"
	$(INSTALL) -m 644 NativeClass.clo "$(datarootdir)"
	$(INSTALL) -m 644 UserClass.clo "$(datarootdir)"
	$(INSTALL) -m 644 IComparable.clo "$(datarootdir)"
	$(INSTALL) -m 644 IComparableMore.clo "$(datarootdir)"
	$(INSTALL) -m 644 IInspectable.clo "$(datarootdir)"
	$(INSTALL) -m 644 IHashKey.clo "$(datarootdir)"
	$(INSTALL) -m 644 ICloneable.clo "$(datarootdir)"
	$(INSTALL) -m 644 IDupeable.clo "$(datarootdir)"
	$(INSTALL) -m 644 ISetValue.clo "$(datarootdir)"
	$(INSTALL) -m 644 ConstructorModule.clm "$(datarootdir)"
	$(INSTALL) -m 644 ComparableModule.clm "$(datarootdir)"
	$(INSTALL) -m 644 OperatorModule.clm "$(datarootdir)"
	$(INSTALL) -m 644 OperatorModuleForDouble.clm "$(datarootdir)"
	$(INSTALL) -m 644 GenericsParam0.clo "$(datarootdir)"
	$(INSTALL) -m 644 GenericsParam1.clo "$(datarootdir)"
	$(INSTALL) -m 644 GenericsParam2.clo "$(datarootdir)"
	$(INSTALL) -m 644 GenericsParam3.clo "$(datarootdir)"
	$(INSTALL) -m 644 GenericsParam4.clo "$(datarootdir)"
	$(INSTALL) -m 644 GenericsParam5.clo "$(datarootdir)"
	$(INSTALL) -m 644 GenericsParam6.clo "$(datarootdir)"
	$(INSTALL) -m 644 GenericsParam7.clo "$(datarootdir)"
	rm -f $(bindir)/clover
	if echo $(CFLAGS) | grep -q MDEBUG; then $(INSTALL) -m 755 clover "$(bindir)"; else $(INSTALL) -s -m 755 clover "$(bindir)"; fi;
	rm -f $(bindir)/cclover
	if echo $(CFLAGS) | grep -q MDEBUG; then $(INSTALL) -m 755 cclover "$(bindir)"; else $(INSTALL) -s -m 755 cclover "$(bindir)"; fi;
	rm -f $(bindir)/iclover
	if echo $(CFLAGS) | grep -q MDEBUG; then $(INSTALL) -m 755 iclover "$(bindir)"; else $(INSTALL) -s -m 755 iclover "$(bindir)"; fi;
	rm -f $(bindir)/psclover
	if echo $(CFLAGS) | grep -q MDEBUG; then $(INSTALL) -m 755 psclover "$(bindir)"; else $(INSTALL) -s -m 755 psclover "$(bindir)"; fi;
	rm -f $(bindir)/llclover
	if echo $(CFLAGS) | grep -q MDEBUG; then $(INSTALL) -m 755 llclover "$(bindir)"; else $(INSTALL) -s -m 755 llclover "$(bindir)"; fi;

dest-install:
	mkdir -p "$(DESTDIR)/$(bindir)";
	mkdir -p "$(DESTDIR)/$(sysconfdir)";
	mkdir -p "$(DESTDIR)/$(libdir)"
	mkdir -p "$(DESTDIR)/$(includedir)"
	mkdir -p "$(DESTDIR)/$(docdir)"
	mkdir -p "$(DESTDIR)/$(mandir)/man1"
	$(INSTALL) -m 644 src/clover.h "$(DESTDIR)/$(includedir)"
	$(INSTALL) -m 644 USAGE "$(DESTDIR)/$(docdir)"
	$(INSTALL) -m 644 README "$(DESTDIR)/$(docdir)"
	$(INSTALL) -m 644 CHANGELOG "$(DESTDIR)/$(docdir)"
	$(INSTALL) -m 644 man/man1/clover.1 "$(DESTDIR)/$(mandir)/man1"
	$(INSTALL) -m 644 Object.clo "$(DESTDIR)/$(datarootdir)"
	$(INSTALL) -m 644 int.clo "$(DESTDIR)/$(datarootdir)"
	$(INSTALL) -m 644 byte.clo "$(DESTDIR)/$(datarootdir)"
	$(INSTALL) -m 644 short.clo "$(DESTDIR)/$(datarootdir)"
	$(INSTALL) -m 644 uint.clo "$(DESTDIR)/$(datarootdir)"
	$(INSTALL) -m 644 long.clo "$(DESTDIR)/$(datarootdir)"
	$(INSTALL) -m 644 char.clo "$(DESTDIR)/$(datarootdir)"
	$(INSTALL) -m 644 float.clo "$(DESTDIR)/$(datarootdir)"
	$(INSTALL) -m 644 double.clo "$(DESTDIR)/$(datarootdir)"
	$(INSTALL) -m 644 bool.clo "$(DESTDIR)/$(datarootdir)"
	$(INSTALL) -m 644 pointer.clo "$(DESTDIR)/$(datarootdir)"
	$(INSTALL) -m 644 Encoding.clo "$(DESTDIR)/$(datarootdir)"
	$(INSTALL) -m 644 Regex.clo "$(DESTDIR)/$(datarootdir)"
	$(INSTALL) -m 644 OnigurumaRegex.clo "$(DESTDIR)/$(datarootdir)"
	$(INSTALL) -m 644 String.clo "$(DESTDIR)/$(datarootdir)"
	$(INSTALL) -m 644 StringBuffer.clo "$(DESTDIR)/$(datarootdir)"
	$(INSTALL) -m 644 Parser.clo "$(DESTDIR)/$(datarootdir)"
	$(INSTALL) -m 644 Bytes.clo "$(DESTDIR)/$(datarootdir)"
	$(INSTALL) -m 644 Range.clo "$(DESTDIR)/$(datarootdir)"
	$(INSTALL) -m 644 'Array$$1.clo' "$(DESTDIR)/$(datarootdir)"
	$(INSTALL) -m 644 'SortableArray$$1.clo' "$(DESTDIR)/$(datarootdir)"
	$(INSTALL) -m 644 'Hash$$2.clo' "$(DESTDIR)/$(datarootdir)"
	$(INSTALL) -m 644 'Tuple$$1.clo' "$(DESTDIR)/$(datarootdir)"
	$(INSTALL) -m 644 'Tuple$$2.clo' "$(DESTDIR)/$(datarootdir)"
	$(INSTALL) -m 644 'Tuple$$3.clo' "$(DESTDIR)/$(datarootdir)"
	$(INSTALL) -m 644 'Tuple$$4.clo' "$(DESTDIR)/$(datarootdir)"
	$(INSTALL) -m 644 'Tuple$$5.clo' "$(DESTDIR)/$(datarootdir)"
	$(INSTALL) -m 644 'Tuple$$6.clo' "$(DESTDIR)/$(datarootdir)"
	$(INSTALL) -m 644 'Tuple$$7.clo' "$(DESTDIR)/$(datarootdir)"
	$(INSTALL) -m 644 'Tuple$$8.clo' "$(DESTDIR)/$(datarootdir)"
	$(INSTALL) -m 644 Field.clo "$(DESTDIR)/$(datarootdir)"
	$(INSTALL) -m 644 Method.clo "$(DESTDIR)/$(datarootdir)"
	$(INSTALL) -m 644 Block.clo "$(DESTDIR)/$(datarootdir)"
	$(INSTALL) -m 644 GenericsParametor.clo "$(DESTDIR)/$(datarootdir)"
	$(INSTALL) -m 644 Class.clo "$(DESTDIR)/$(datarootdir)"
	$(INSTALL) -m 644 Type.clo "$(DESTDIR)/$(datarootdir)"
	$(INSTALL) -m 644 Enum.clo "$(DESTDIR)/$(datarootdir)"
	$(INSTALL) -m 644 Null.clo "$(DESTDIR)/$(datarootdir)"
	$(INSTALL) -m 644 anonymous.clo "$(DESTDIR)/$(datarootdir)"
	$(INSTALL) -m 644 void.clo "$(DESTDIR)/$(datarootdir)"
	$(INSTALL) -m 644 Thread.clo "$(DESTDIR)/$(datarootdir)"
	$(INSTALL) -m 644 Mutex.clo "$(DESTDIR)/$(datarootdir)"
	$(INSTALL) -m 644 Clover.clo "$(DESTDIR)/$(datarootdir)"
	$(INSTALL) -m 644 System.clo "$(DESTDIR)/$(datarootdir)"
	$(INSTALL) -m 644 Exception.clo "$(DESTDIR)/$(datarootdir)"
	$(INSTALL) -m 644 SystemException.clo "$(DESTDIR)/$(datarootdir)"
	$(INSTALL) -m 644 NullPointerException.clo "$(DESTDIR)/$(datarootdir)"
	$(INSTALL) -m 644 RangeException.clo "$(DESTDIR)/$(datarootdir)"
	$(INSTALL) -m 644 ConvertingStringCodeException.clo "$(DESTDIR)/$(datarootdir)"
	$(INSTALL) -m 644 ClassNotFoundException.clo "$(DESTDIR)/$(datarootdir)"
	$(INSTALL) -m 644 IOException.clo "$(DESTDIR)/$(datarootdir)"
	$(INSTALL) -m 644 OverflowException.clo "$(DESTDIR)/$(datarootdir)"
	$(INSTALL) -m 644 CantSolveGenericsType.clo "$(DESTDIR)/$(datarootdir)"
	$(INSTALL) -m 644 TypeError.clo "$(DESTDIR)/$(datarootdir)"
	$(INSTALL) -m 644 MethodMissingException.clo "$(DESTDIR)/$(datarootdir)"
	$(INSTALL) -m 644 DivisionByZeroException.clo "$(DESTDIR)/$(datarootdir)"
	$(INSTALL) -m 644 OverflowStackSizeException.clo "$(DESTDIR)/$(datarootdir)"
	$(INSTALL) -m 644 InvalidRegexException.clo "$(DESTDIR)/$(datarootdir)"
	$(INSTALL) -m 644 KeyNotFoundException.clo "$(DESTDIR)/$(datarootdir)"
	$(INSTALL) -m 644 KeyOverlappingException.clo "$(DESTDIR)/$(datarootdir)"
	$(INSTALL) -m 644 OutOfRangeOfStackException.clo "$(DESTDIR)/$(datarootdir)"
	$(INSTALL) -m 644 OutOfRangeOfFieldException.clo "$(DESTDIR)/$(datarootdir)"
	$(INSTALL) -m 644 NativeClass.clo "$(DESTDIR)/$(datarootdir)"
	$(INSTALL) -m 644 UserClass.clo "$(DESTDIR)/$(datarootdir)"
	$(INSTALL) -m 644 IComparable.clo "$(DESTDIR)/$(datarootdir)"
	$(INSTALL) -m 644 IComparableMore.clo "$(DESTDIR)/$(datarootdir)"
	$(INSTALL) -m 644 IInspectable.clo "$(DESTDIR)/$(datarootdir)"
	$(INSTALL) -m 644 IHashKey.clo "$(DESTDIR)/$(datarootdir)"
	$(INSTALL) -m 644 ICloneable.clo "$(DESTDIR)/$(datarootdir)"
	$(INSTALL) -m 644 IDupeable.clo "$(DESTDIR)/$(datarootdir)"
	$(INSTALL) -m 644 ISetValue.clo "$(DESTDIR)/$(datarootdir)"
	$(INSTALL) -m 644 ConstructorModule.clm "$(DESTDIR)/$(datarootdir)"
	$(INSTALL) -m 644 ComparableModule.clm "$(DESTDIR)/$(datarootdir)"
	$(INSTALL) -m 644 OperatorModule.clm "$(DESTDIR)/$(datarootdir)"
	$(INSTALL) -m 644 OperatorModuleForDouble.clm "$(DESTDIR)/$(datarootdir)"
	$(INSTALL) -m 644 GenericsParam0.clo "$(DESTDIR)/$(datarootdir)"
	$(INSTALL) -m 644 GenericsParam1.clo "$(DESTDIR)/$(datarootdir)"
	$(INSTALL) -m 644 GenericsParam2.clo "$(DESTDIR)/$(datarootdir)"
	$(INSTALL) -m 644 GenericsParam3.clo "$(DESTDIR)/$(datarootdir)"
	$(INSTALL) -m 644 GenericsParam4.clo "$(DESTDIR)/$(datarootdir)"
	$(INSTALL) -m 644 GenericsParam5.clo "$(DESTDIR)/$(datarootdir)"
	$(INSTALL) -m 644 GenericsParam6.clo "$(DESTDIR)/$(datarootdir)"
	$(INSTALL) -m 644 GenericsParam7.clo "$(DESTDIR)/$(datarootdir)"
	rm -f "$(DESTDIR)/$(bindir)/clover"
	if echo $(CFLAGS) | grep -q MDEBUG; then $(INSTALL) -m 755 clover "$(DESTDIR)/$(bindir)"; else $(INSTALL) -s -m 755 clover "$(DESTDIR)/$(bindir)"; fi;
	rm -f "$(DESTDIR)/$(bindir)/cclover"
	if echo $(CFLAGS) | grep -q MDEBUG; then $(INSTALL) -m 755 cclover "$(DESTDIR)/$(bindir)"; else $(INSTALL) -s -m 755 cclover "$(DESTDIR)/$(bindir)"; fi;
	rm -f "$(DESTDIR)/$(bindir)/iclover"
	if echo $(CFLAGS) | grep -q MDEBUG; then $(INSTALL) -m 755 iclover "$(DESTDIR)/$(bindir)"; else $(INSTALL) -s -m 755 iclover "$(DESTDIR)/$(bindir)"; fi;
	rm -f "$(DESTDIR)/$(bindir)/psclover"
	if echo $(CFLAGS) | grep -q MDEBUG; then $(INSTALL) -m 755 psclover "$(DESTDIR)/$(bindir)"; else $(INSTALL) -s -m 755 psclover "$(DESTDIR)/$(bindir)"; fi;
	rm -f "$(DESTDIR)/$(bindir)/llclover"
	if echo $(CFLAGS) | grep -q MDEBUG; then $(INSTALL) -m 755 llclover "$(DESTDIR)/$(bindir)"; else $(INSTALL) -s -m 755 llclover "$(DESTDIR)/$(bindir)"; fi;

#########################################################
# uninstall
#########################################################
uninstall:
	rm -Rf ~/.clover
	rm -f $(includedir)/clover.h
	rm -rf $(includedir)
	rm -f $(docdir)/USAGE
	rm -f $(docdir)/README
	rm -f $(docdir)/CHANGELOG
	rm -rf $(docdir)
	rm -f $(libdir)/libclover*
	rm -f $(mandir)/man1/clover.1
	rm -f $(datarootdir)/*.clo $(datarootdir)/*.clm
	rm -fr $(datarootdir)
	rm -f $(bindir)/clover
	rm -f $(bindir)/iclover
	rm -f $(bindir)/psclover
	rm -f $(bindir)/lllover
	rm -f $(bindir)/cclover

#########################################################
# permission
#########################################################
permission:
	chmod 644 *
	chmod 755 .git man src configure
	chmod 644 src/*.c
	chmod 644 src/*.h

#########################################################
# test
#########################################################
test: test-compile test-body

test2: test2-compile test2-body

test-compile:
	@echo "Compile to test code..."
	./cclover code/hello_world.cl
	./cclover code/hello_world2.cl
	./cclover code/accessor.clc
	./cclover code/accessor.cl
	./cclover code/load_class_test.clc
	./cclover code/load_class_test.cl
	./cclover code/new.clc
	./cclover code/new.cl
	./cclover code/extends.clc
	./cclover code/extends.cl
	./cclover code/open_class.clc
	./cclover code/open_class.cl
	./cclover code/operator.clc
	./cclover code/operator.cl
	./cclover code/if_test.cl
	./cclover code/method_with_block.clc
	./cclover code/method_with_block.cl
	./cclover code/while_test.cl
	./cclover code/static_method_block.clc
	./cclover code/static_method_block.cl
	./cclover code/exception_simple.cl
	./cclover code/exception.clc
	./cclover code/exception.cl
	./cclover code/exception2.clc
	./cclover code/exception2.cl
	./cclover code/instanceof.cl
	./cclover code/namespace.clc
	./cclover code/namespace.cl
	./cclover code/revert_from_method_block.clc
	./cclover code/revert_from_method_block.cl
	./cclover code/super_with_block.clc
	./cclover code/super_with_block.cl
	./cclover code/block_test.cl
	./cclover code/try.cl
	./cclover code/qmark_operator.cl
	./cclover code/continue_test.cl
	./cclover code/output_to_s.cl
	./cclover code/new_test.clc
	./cclover code/new_test.cl
	./cclover code/inherit_with_block.clc
	./cclover code/inherit_with_block.cl
	./cclover code/string_test.cl
	./cclover code/field_initializer.clc
	./cclover code/field_initializer.cl 
	./cclover code/field_initializer2.clc
	./cclover code/field_initializer2.cl
	./cclover code/field_initializer3.clc
	./cclover code/field_initializer3.cl
	./cclover code/int.cl
	./cclover code/operand.clc
	./cclover code/operand.cl
	./cclover code/thread.cl
	./cclover code/thread2.cl
	./cclover code/string_test2.clc
	./cclover code/string_test2.cl
	./cclover code/string_test3.cl
	./cclover code/operator2.clc
	./cclover code/operator2.cl
	./cclover code/byte.cl
	./cclover code/bytes.cl
	./cclover code/param_initializer.clc
	./cclover code/param_initializer.cl 
	./cclover code/param_initializer2.clc
	./cclover code/param_initializer2.cl 
	./cclover code/free_order_on_class_definition3.clc 
	./cclover code/free_order_on_class_definition.clc
	./cclover  code/free_order_on_class_definition.cl
	./cclover code/mixin.clc
	./cclover code/mixin.cl
	./cclover code/interface.clc
	./cclover code/interface.cl
	./cclover code/abstract_class_test.clc
	./cclover code/abstract_class_test.cl
	./cclover code/array.cl
	./cclover code/hash.cl
	./cclover code/interface2.clc
	./cclover code/interface2.cl
	./cclover code/generics2.clc
	./cclover code/generics2.cl
	./cclover code/generics3.clc
	./cclover code/generics3.cl
	./cclover code/generics3-2.clc
	./cclover code/generics3-2.cl
	./cclover code/generics4.clc
	./cclover code/generics4.cl
	./cclover code/generics5.clc
	./cclover code/generics5.cl
	./cclover code/generics6.clc
	./cclover code/generics6.cl
	./cclover code/generics7.clc
	./cclover code/generics7.cl
	./cclover code/generics8.clc
	./cclover code/generics8.cl
	./cclover code/generics9.clc
	./cclover code/generics9.cl
	./cclover code/generics9-0.clc
	./cclover code/generics9-0.cl
	./cclover code/generics9-1.clc
	./cclover code/generics9-1.cl
	./cclover code/generics9-2.clc
	./cclover code/generics9-2.cl
	./cclover code/generics10.clc
	./cclover code/generics10.cl
	./cclover code/generics10-1.clc
	./cclover code/generics10-1.cl
	./cclover code/generics10-2.clc
	./cclover code/generics10-2.cl
	./cclover code/generics10-3.clc
	./cclover code/generics10-3.cl
	./cclover code/generics10-4.clc
	./cclover code/generics10-4.cl
	./cclover code/generics10-5.clc
	./cclover code/generics10-5.cl
	./cclover code/generics10-6.clc
	./cclover code/generics10-6.cl
	./cclover code/generics11.clc
	./cclover code/generics11.cl
	./cclover code/generics12.clc
	./cclover code/generics12.cl
	./cclover code/generics12-1.clc
	./cclover code/generics12-1.cl
	./cclover code/generics12-2.clc
	./cclover code/generics12-2.cl
	./cclover code/generics12-3.clc
	./cclover code/generics12-3.cl
	./cclover code/generics12-4.clc
	./cclover code/generics12-4.cl
	./cclover code/generics15.clc
	./cclover code/generics15.cl
	./cclover code/generics16.clc
	./cclover code/generics16.cl
	./cclover code/generics17.clc
	./cclover code/generics17.cl
	./cclover code/generics17-2.clc
	./cclover code/generics17-2.cl
	./cclover code/generics18.clc
	./cclover code/generics18.cl
	./cclover code/generics18-2.clc
	./cclover code/generics19.clc
	./cclover code/generics19.cl
	./cclover code/generics20.clc
	./cclover code/generics20.cl
	./cclover code/generics21.clc
	./cclover code/generics21.cl
	./cclover code/generics22.clc
	./cclover code/generics22.cl
	./cclover code/generics23.clc
	./cclover code/generics23.cl
	./cclover code/generics24.clc
	./cclover code/generics24.cl
	./cclover code/generics25.clc
	./cclover code/generics25.cl
	./cclover code/generics26.clc
	./cclover code/generics27.clc
	./cclover code/generics28.clc
	./cclover code/generics28.cl
	./cclover code/generics.clc
	./cclover code/generics.cl
	./cclover code/array2.cl
	./cclover code/dynamic_typing.clc
	./cclover code/dynamic_typing.cl
	./cclover code/my_int.clc
	./cclover code/my_int.cl
	./cclover code/my_array.clc
	./cclover code/my_array.cl
	./cclover code/type.cl
	./cclover code/type2.cl
	./cclover code/null2.cl
	./cclover code/object.cl
	./cclover code/method_parametor_test.clc
	./cclover code/method_parametor_test.cl
	./cclover code/protected_test.clc
	./cclover code/dynamic_typing2.clc
	./cclover code/dynamic_typing2.cl
	./cclover code/try2.cl
	./cclover code/if_test2.cl
	./cclover code/for.cl
	./cclover code/block_test2.cl
	./cclover code/block_test3.cl
	./cclover code/method_with_block3.clc
	./cclover code/method_with_block3.cl
	./cclover code/method_with_block4.clc
	./cclover code/method_with_block4.cl
	./cclover code/method_with_block5.cl
	./cclover code/method_with_block6.clc
	./cclover code/method_with_block6.cl
	./cclover code/method_with_block7.clc
	./cclover code/method_with_block7.cl
	./cclover code/block_with_method_and_for.cl
	./cclover code/call_by_value_and_ref.clc
	./cclover code/call_by_value_and_ref.cl 
	./cclover code/ModuleTest.clc
	./cclover code/ModuleTest2.clc
	./cclover code/ModuleTest.cl
	./cclover code/CallByValueTest.clc
	./cclover code/CallByValueTest.cl
	./cclover code/array3.cl
	./cclover code/array4.clc
	./cclover code/array4.cl
	./cclover code/array5.cl 
	./cclover code/array6.cl 
	./cclover code/VariableArguments.clc
	./cclover code/VariableArguments.cl
	./cclover code/class_method.clc
	./cclover code/class_method.cl
	./cclover code/class_method_and_field.clc
	./cclover code/class_method_and_field.cl
	./cclover code/tuple1.cl
	./cclover code/class_object1.cl
	./cclover code/class_object2.clc
	./cclover code/class_object2.cl
	./cclover code/reflection1.clc
	./cclover code/reflection1.cl
	./cclover code/enum.clc
	./cclover code/enum.cl
	./cclover code/enum2.clc
	./cclover code/enum2.cl
	./cclover code/oror_andand.cl
	./cclover code/field_test1.clc
	./cclover code/field_test1.cl
	./cclover code/field_test2.clc
	./cclover code/field_test2b.clc
	./cclover code/field_test2.cl
	./cclover code/multiple_assinment.clc
	./cclover code/multiple_assinment.cl
	./cclover code/method_block_break.clc
	./cclover code/method_block_break.cl
	./cclover code/string_range.cl 
	./cclover code/method_missing.clc
	./cclover code/method_missing.cl
	./cclover code/class_name_test.clc
	./cclover code/class_name_test.cl
	./cclover code/enum3.clc
	./cclover code/enum3.cl
	./cclover code/enum4.cl
	./cclover code/enum5.clc
	./cclover code/enum5.cl
	./cclover code/open_test.cl
	./cclover code/duck_typing_test.clc
	./cclover code/duck_typing_test.cl
	./cclover code/command_test.cl
	./cclover code/command_test2.cl
	./cclover code/native_class.cl
	./pclover code/preprocessor.cl.in
	./cclover code/preprocessor.cl
	./pclover code/preprocessor2.cl.in
	./cclover code/preprocessor2.cl
	./cclover code/number.cl
	./cclover code/char_test.cl
	./cclover code/double_test.cl
	./cclover code/mixin_test3-1.clc
	./cclover code/mixin_test3-2.clc
	./cclover code/mixin_test3-3.clc
	./cclover code/mixin_test3.cl
	./cclover code/printf.cl
	./cclover code/time.cl
	./cclover code/regex.cl
	./cclover code/file2.cl
	./cclover code/directory.cl
	./cclover code/termios.cl
	./cclover code/generics_test_a.clc
	./cclover code/generics_test_a.cl
	./cclover code/argv_test.cl
	./cclover code/setenv.cl
	./cclover code/realpath.cl
	./cclover code/umask.cl
	./cclover code/id.cl
	./cclover code/object_field1.clc
	./cclover code/object_field1.cl
	./cclover code/object_field2.clc
	./cclover code/object_field2.cl
	./cclover code/instanceof2.clc
	./cclover code/instanceof2.cl
	./cclover code/is_child_test.clc
	./cclover code/is_child_test.cl
	./cclover code/class_object3.clc
	./cclover code/class_object3.cl
	./cclover code/class_object4.clc
	./cclover code/class_object4.cl
	./cclover code/class_object5.clc
	./cclover code/class_object5.cl
	./cclover code/field_test3.clc
	./cclover code/field_test3.cl
	./cclover code/field_test4.clc
	./cclover code/field_test4.cl
	./cclover code/method_test.clc
	./cclover code/method_test.cl
	./cclover code/method_test2.clc
	./cclover code/method_test2.cl
	./cclover code/type3.cl
	./cclover code/constructor_test.clc
	./cclover code/constructor_test.cl
	./cclover code/constructor_test2.clc
	./cclover code/constructor_test2.cl
	./cclover code/parser.cl
	./cclover code/string_buffer.cl
	./cclover code/caller_test.cl
	./cclover code/fundamental_class_test_code.clc
	./cclover code/fundamental_class_test_code.cl
	./cclover code/stdlib_test.cl

test2-compile:
	@echo "Compile to test code..."
	./cclover code2/my_int.clc
	./cclover code2/my_int.cl
	./cclover code2/my_array.clc
	./cclover code2/my_array.cl

test-body:
	@echo "Start to test and running code..."
	if test -e output_of_time; then mv -f output_of_time output_of_time.before; fi
	@echo "--- hello_world.cl ---"
	@echo "--- hello_world.cl ---" >> output_of_time
	/usr/bin/time ./clover code/hello_world.cl 2>> output_of_time
	@echo "--- hello_world2.cl ---"
	@echo "--- hello_world2.cl ---" >> output_of_time
	/usr/bin/time ./clover code/hello_world2.cl 2>> output_of_time
	@echo "--- accessor.cl ---"
	@echo "--- accessor.cl ---" >> output_of_time
	/usr/bin/time ./clover code/accessor.cl 2>> output_of_time
	@echo "--- load_class_test.cl ---"
	@echo "--- load_class_test.cl ---" >> output_of_time
	/usr/bin/time ./clover code/load_class_test.cl 2>> output_of_time
	@echo "--- new.cl ---"
	@echo "--- new.cl ---" >> output_of_time
	/usr/bin/time ./clover code/new.cl 2>> output_of_time
	@echo "--- extends.cl ---"
	@echo "--- extends.cl ---" >> output_of_time
	/usr/bin/time ./clover code/extends.cl 2>> output_of_time
	@echo "--- open_class ---"
	@echo "--- open_class ---" >> output_of_time
	/usr/bin/time ./clover code/open_class.cl 2>> output_of_time
	@echo "--- operator.cl ---"
	@echo "--- operator.cl ---" >> output_of_time
	/usr/bin/time ./clover code/operator.cl 2>> output_of_time
	@echo "--- if_test.cl ---"
	@echo "--- if_test.cl ---" >> output_of_time
	/usr/bin/time ./clover code/if_test.cl 2>> output_of_time
	@echo "--- method_with_block.cl ---"
	@echo "--- method_with_block.cl ---" >> output_of_time
	/usr/bin/time ./clover code/method_with_block.cl 2>> output_of_time
	@echo "--- while_test.cl ---"
	@echo "--- while_test.cl ---" >> output_of_time
	/usr/bin/time ./clover code/while_test.cl 2>> output_of_time
	@echo "--- static_method_block.cl ---"
	@echo "--- static_method_block.cl ---" >> output_of_time
	/usr/bin/time ./clover code/static_method_block.cl 2>> output_of_time
	@echo "--- exception_simple.cl ---"
	@echo "--- exception_simple.cl ---" >> output_of_time
	/usr/bin/time ./clover code/exception_simple.cl 2>> output_of_time
	@echo "--- exception.cl ---"
	@echo "--- exception.cl ---" >> output_of_time
	/usr/bin/time ./clover code/exception.cl 2>> output_of_time
	@echo "--- exception2.cl ---"
	@echo "--- exception2.cl ---" >> output_of_time
	/usr/bin/time ./clover code/exception2.cl 2>> output_of_time
	@echo "--- instanceof.cl ---"
	@echo "--- instanceof.cl ---" >> output_of_time
	/usr/bin/time ./clover code/instanceof.cl 2>> output_of_time
	@echo "--- namespace.cl ---"
	@echo "--- namespace.cl ---" >> output_of_time
	/usr/bin/time ./clover code/namespace.cl 2>> output_of_time
	@echo "--- revert_from_method_block.cl ---"
	@echo "--- revert_from_method_block.cl ---" >> output_of_time
	/usr/bin/time ./clover code/revert_from_method_block.cl 2>> output_of_time
	@echo "--- super_with_block.cl ---"
	@echo "--- super_with_block.cl ---" >> output_of_time
	/usr/bin/time ./clover code/super_with_block.cl 2>> output_of_time
	@echo "--- block_test.cl ---"
	@echo "--- block_test.cl ---" >> output_of_time
	/usr/bin/time ./clover code/block_test.cl 2>> output_of_time
	@echo "--- try.cl ---"
	@echo "--- try.cl ---" >> output_of_time
	/usr/bin/time ./clover code/try.cl 2>> output_of_time
	@echo "--- qmark_operator.cl ---"
	@echo "--- qmark_operator.cl ---" >> output_of_time
	/usr/bin/time ./clover code/qmark_operator.cl 2>> output_of_time
	@echo "--- continue_test.cl ---"
	@echo "--- continue_test.cl ---" >> output_of_time
	/usr/bin/time ./clover code/continue_test.cl 2>> output_of_time
	@echo "--- output_to_s.cl ---"
	@echo "--- output_to_s.cl ---" >> output_of_time
	/usr/bin/time ./clover code/output_to_s.cl 2>> output_of_time
	@echo "--- new_test.cl ---"
	@echo "--- new_test.cl ---" >> output_of_time
	/usr/bin/time ./clover code/new_test.cl 2>> output_of_time
	@echo "--- inherit_with_block.cl ---"
	@echo "--- inherit_with_block.cl ---" >> output_of_time
	/usr/bin/time ./clover code/inherit_with_block.cl 2>> output_of_time
	@echo "--- string_test.cl ---"
	@echo "--- string_test.cl ---" >> output_of_time
	/usr/bin/time ./clover code/string_test.cl 2>> output_of_time
	@echo "--- field_initializer.cl ---"
	@echo "--- field_initializer.cl ---" >> output_of_time
	/usr/bin/time ./clover code/field_initializer.cl 2>> output_of_time
	@echo "--- field_initializer2.cl ---"
	@echo "--- field_initializer2.cl ---" >> output_of_time
	/usr/bin/time ./clover code/field_initializer2.cl 2>> output_of_time
	@echo "--- field_initializer3.cl ---"
	@echo "--- field_initializer3.cl ---" >> output_of_time
	/usr/bin/time ./clover code/field_initializer3.cl 2>> output_of_time
	@echo "--- int.cl ---"
	@echo "--- int.cl ---" >> output_of_time
	/usr/bin/time ./clover code/int.cl 2>> output_of_time
	@echo "--- thread.cl ---"
	@echo "--- thread.cl ---" >> output_of_time
	/usr/bin/time ./clover code/thread.cl 2>> output_of_time
	@echo "--- thread2.cl ---"
	@echo "--- thread2.cl ---" >> output_of_time
	/usr/bin/time ./clover code/thread2.cl 2>> output_of_time
	@echo "--- string_test2.cl ---"
	@echo "--- string_test2.cl ---" >> output_of_time
	/usr/bin/time ./clover code/string_test2.cl 2>> output_of_time
	@echo "--- byte.cl ---"
	@echo "--- byte.cl ---" >> output_of_time
	/usr/bin/time ./clover code/byte.cl 2>> output_of_time
	@echo "--- bytes.cl ---"
	@echo "--- bytes.cl ---" >> output_of_time
	/usr/bin/time ./clover code/bytes.cl 2>> output_of_time

	@echo "--- param_initializer.cl ---"
	@echo "--- param_initializer.cl ---" >> output_of_time
	/usr/bin/time ./clover code/param_initializer.cl 2>> output_of_time

	@echo "--- param_initializer2.cl ---"
	@echo "--- param_initializer2.cl ---" >> output_of_time
	/usr/bin/time ./clover code/param_initializer2.cl 2>> output_of_time

	@echo "--- free_order_on_class_definition.cl ---"
	@echo "--- free_order_on_class_definition.cl ---" >> output_of_time
	/usr/bin/time ./clover code/free_order_on_class_definition.cl 2>> output_of_time

	@echo "--- operand.cl ---"
	@echo "--- operand.cl ---" >> output_of_time
	/usr/bin/time ./clover code/operand.cl 2>> output_of_time

	@echo "--- mixin ---"
	@echo "--- mixin ---" >> output_of_time
	/usr/bin/time ./clover code/mixin.cl 2>> output_of_time

	@echo "--- interface ---"
	@echo "--- interface ---" >> output_of_time
	/usr/bin/time ./clover code/interface.cl 2>> output_of_time

	@echo "--- abstract class ---"
	@echo "--- abstract class ---" >> output_of_time
	/usr/bin/time ./clover code/abstract_class_test.cl 2>> output_of_time

	@echo "-- interface2 ---"
	@echo "--- interface2 ---" >> output_of_time
	/usr/bin/time ./clover code/interface2.cl 2>> output_of_time

	@echo "-- generics2 ---"
	@echo "--- generics2 ---" >> output_of_time
	/usr/bin/time ./clover code/generics2.cl 2>> output_of_time

	@echo "-- generics3 ---"
	@echo "--- generics3 ---" >> output_of_time
	/usr/bin/time ./clover code/generics3.cl 2>> output_of_time

	@echo "--- generics3-2 ---"
	@echo "--- generics3-2 ---" >> output_of_time
	/usr/bin/time ./clover code/generics3-2.cl 2>> output_of_time

	@echo "--- generics4 ---"
	@echo "--- generics4 ---" >> output_of_time
	/usr/bin/time ./clover code/generics4.cl 2>> output_of_time

	@echo "--- generics5 ---"
	@echo "--- generics5 ---" >> output_of_time
	/usr/bin/time ./clover code/generics5.cl 2>> output_of_time

	@echo "--- generics6 ---"
	@echo "--- generics6 ---" >> output_of_time
	/usr/bin/time ./clover code/generics6.cl 2>> output_of_time

	@echo "--- generics7 ---"
	@echo "--- generics7 ---" >> output_of_time
	/usr/bin/time ./clover code/generics7.cl 2>> output_of_time

	@echo "--- generics8 ---"
	@echo "--- generics8 ---" >> output_of_time
	/usr/bin/time ./clover code/generics8.cl 2>> output_of_time

	@echo "--- generics9 ---"
	@echo "--- generics9 ---" >> output_of_time
	/usr/bin/time ./clover code/generics9.cl 2>> output_of_time

	@echo "--- generics9-0 ---"
	@echo "--- generics9-0 ---" >> output_of_time
	/usr/bin/time ./clover code/generics9-0.cl 2>> output_of_time

	@echo "--- generics9-1 ---"
	@echo "--- generics9-1 ---" >> output_of_time
	/usr/bin/time ./clover code/generics9-1.cl 2>> output_of_time

	@echo "--- generics9-2 ---"
	@echo "--- generics9-2 ---" >> output_of_time
	/usr/bin/time ./clover code/generics9-2.cl 2>> output_of_time

	@echo "--- generics10 ---"
	@echo "--- generics10 ---" >> output_of_time
	/usr/bin/time ./clover code/generics10.cl 2>> output_of_time

	@echo "--- generics10-1 ---"
	@echo "--- generics10-1 ---" >> output_of_time
	/usr/bin/time ./clover code/generics10-1.cl 2>> output_of_time

	@echo "--- generics10-2 ---"
	@echo "--- generics10-2 ---" >> output_of_time
	/usr/bin/time ./clover code/generics10-2.cl 2>> output_of_time

	@echo "--- generics10-3 ---"
	@echo "--- generics10-3 ---" >> output_of_time
	/usr/bin/time ./clover code/generics10-3.cl 2>> output_of_time

	@echo "--- generics10-4 ---"
	@echo "--- generics10-4 ---" >> output_of_time
	/usr/bin/time ./clover code/generics10-4.cl 2>> output_of_time

	@echo "--- generics10-5 ---"
	@echo "--- generics10-5 ---" >> output_of_time
	/usr/bin/time ./clover code/generics10-5.cl 2>> output_of_time

	@echo "--- generics10-6 ---"
	@echo "--- generics10-6 ---" >> output_of_time
	/usr/bin/time ./clover code/generics10-6.cl 2>> output_of_time

	@echo "--- generics11 ---"
	@echo "--- generics11 ---" >> output_of_time
	/usr/bin/time ./clover code/generics11.cl 2>> output_of_time

	@echo "--- generics12 ---"
	@echo "--- generics12 ---" >> output_of_time
	/usr/bin/time ./clover code/generics12.cl 2>> output_of_time

	@echo "--- generics12-1 ---"
	@echo "--- generics12-1 ---" >> output_of_time
	/usr/bin/time ./clover code/generics12-1.cl 2>> output_of_time

	@echo "--- generics12-2 ---"
	@echo "--- generics12-2 ---" >> output_of_time
	/usr/bin/time ./clover code/generics12-2.cl 2>> output_of_time

	@echo "--- generics12-3 ---"
	@echo "--- generics12-3 ---" >> output_of_time
	/usr/bin/time ./clover code/generics12-3.cl 2>> output_of_time

	@echo "--- generics12-4 ---"
	@echo "--- generics12-4 ---" >> output_of_time
	/usr/bin/time ./clover code/generics12-4.cl 2>> output_of_time

	@echo "--- generics15 ---"
	@echo "--- generics15 ---" >> output_of_time
	/usr/bin/time ./clover code/generics15.cl 2>> output_of_time

	@echo "--- generics16 ---"
	@echo "--- generics16 ---" >> output_of_time
	/usr/bin/time ./clover code/generics16.cl 2>> output_of_time

	@echo "-- generics17 ---"
	@echo "--- generics17 ---" >> output_of_time
	/usr/bin/time ./clover code/generics17.cl 2>> output_of_time

	@echo "-- generics17-2 ---"
	@echo "--- generics17-2 ---" >> output_of_time
	/usr/bin/time ./clover code/generics17-2.cl 2>> output_of_time

	@echo "-- generics18 ---"
	@echo "--- generics18 ---" >> output_of_time
	/usr/bin/time ./clover code/generics18.cl 2>> output_of_time

	@echo "-- generics19 ---"
	@echo "--- generics19 ---" >> output_of_time
	/usr/bin/time ./clover code/generics19.cl 2>> output_of_time

	@echo "-- generics20 ---"
	@echo "--- generics20 ---" >> output_of_time
	/usr/bin/time ./clover code/generics20.cl 2>> output_of_time

	@echo "-- generics21 ---"
	@echo "--- generics21 ---" >> output_of_time
	/usr/bin/time ./clover code/generics21.cl 2>> output_of_time

	@echo "-- generics22 ---"
	@echo "--- generics22 ---" >> output_of_time
	/usr/bin/time ./clover code/generics22.cl 2>> output_of_time

	@echo "-- generics23 ---"
	@echo "--- generics23 ---" >> output_of_time
	/usr/bin/time ./clover code/generics23.cl 2>> output_of_time

	@echo "-- generics24 ---"
	@echo "--- generics24 ---" >> output_of_time
	/usr/bin/time ./clover code/generics24.cl 2>> output_of_time

	@echo "-- generics25 ---"
	@echo "--- generics25 ---" >> output_of_time
	/usr/bin/time ./clover code/generics25.cl 2>> output_of_time

	@echo "-- generics28 ---"
	@echo "--- generics28 ---" >> output_of_time
	/usr/bin/time ./clover code/generics28.cl 2>> output_of_time

	@echo "--- operator2.cl ---"
	@echo "--- operator2.cl ---" >> output_of_time
	/usr/bin/time ./clover code/operator2.cl 2>> output_of_time

	@echo "--- generics.cl ---"
	@echo "--- generics.cl ---" >> output_of_time
	/usr/bin/time ./clover code/generics.cl 2>> output_of_time

	@echo "--- Array ---"
	@echo "--- Array ---" >> output_of_time
	/usr/bin/time ./clover code/array.cl 2>> output_of_time

	@echo "--- Array2 ---"
	@echo "--- Array2 ---" >> output_of_time
	/usr/bin/time ./clover code/array2.cl 2>> output_of_time

	@echo "--- Hash ---"
	@echo "--- Hash ---" >> output_of_time
	/usr/bin/time ./clover code/hash.cl 2>> output_of_time

	@echo "--- dynamic typing ---"
	@echo "--- dynamic typing ---" >> output_of_time
	/usr/bin/time ./clover code/dynamic_typing.cl 2>> output_of_time

	@echo "--- my_int ---"
	@echo "--- my_int ---" >> output_of_time
	/usr/bin/time ./clover code/my_int.cl 2>> output_of_time

	@echo "--- my_array ---"
	@echo "--- my_array ---" >> output_of_time
	/usr/bin/time ./clover code/my_array.cl 2>> output_of_time

	@echo "--- type ---"
	@echo "--- type ---" >> output_of_time
	/usr/bin/time ./clover code/type.cl 2>> output_of_time

	@echo "--- type2 ---"
	@echo "--- type2 ---" >> output_of_time
	/usr/bin/time ./clover code/type2.cl 2>> output_of_time

	@echo "--- null2 ---"
	@echo "--- null2 ---" >> output_of_time
	/usr/bin/time ./clover code/null2.cl 2>> output_of_time

	@echo "--- object ---"
	@echo "--- object ---" >> output_of_time
	/usr/bin/time ./clover code/object.cl 2>> output_of_time

	@echo "--- method_parametor_test ---"
	@echo "--- method_parametor_test ---" >> output_of_time
	/usr/bin/time ./clover code/method_parametor_test.cl 2>> output_of_time

	@echo "--- dynamic_typing2 ---"
	@echo "--- dynamic_typing2 ---" >> output_of_time
	/usr/bin/time ./clover code/dynamic_typing2.cl 2>> output_of_time

	@echo "--- try2.cl ---"
	@echo "--- try2.cl ---" >> output_of_time
	/usr/bin/time ./clover code/try2.cl 2>> output_of_time

	@echo "--- if_test2.cl ---"
	@echo "--- if_test2.cl ---" >> output_of_time
	/usr/bin/time ./clover code/if_test2.cl 2>> output_of_time

	@echo "--- for.cl ---"
	@echo "--- for.cl ---" >> output_of_time
	/usr/bin/time ./clover code/for.cl 2>> output_of_time

	@echo "--- block_test2.cl ---"
	@echo "--- block_test2.cl ---" >> output_of_time
	/usr/bin/time ./clover code/block_test2.cl 2>> output_of_time

	@echo "--- block_test3.cl ---"
	@echo "--- block_test3.cl ---" >> output_of_time
	/usr/bin/time ./clover code/block_test3.cl 2>> output_of_time

	@echo "--- method_with_block3.cl ---"
	@echo "--- method_with_block3.cl ---" >> output_of_time
	/usr/bin/time ./clover code/method_with_block3.cl 2>> output_of_time

	@echo "--- method_with_block4.cl ---"
	@echo "--- method_with_block4.cl ---" >> output_of_time
	/usr/bin/time ./clover code/method_with_block4.cl 2>> output_of_time

	@echo "--- method_with_block5.cl ---"
	@echo "--- method_with_block5.cl ---" >> output_of_time
	/usr/bin/time ./clover code/method_with_block5.cl 2>> output_of_time

	@echo "--- method_with_block6.cl ---"
	@echo "--- method_with_block6.cl ---" >> output_of_time
	/usr/bin/time ./clover code/method_with_block6.cl 2>> output_of_time

	@echo "--- method_with_block7.cl ---"
	@echo "--- method_with_block7.cl ---" >> output_of_time
	/usr/bin/time ./clover code/method_with_block7.cl 2>> output_of_time

	@echo "--- block_with_method_and_for.cl ---"
	@echo "--- block_with_method_and_for.cl ---" >> output_of_time
	/usr/bin/time ./clover code/block_with_method_and_for.cl 2>> output_of_time

	@echo "--- call_by_value_and_ref.cl ---"
	@echo "--- call_by_value_and_ref.cl.cl ---" >> output_of_time
	/usr/bin/time ./clover code/call_by_value_and_ref.cl 2>> output_of_time

	@echo "--- ModuleTest.cl ---"
	@echo "--- ModuleTest.cl ---" >> output_of_time
	/usr/bin/time ./clover code/ModuleTest.cl 2>> output_of_time

	@echo "--- CallByValueTest.cl ---"
	@echo "--- CallByValueTest.cl ---" >> output_of_time
	/usr/bin/time ./clover code/CallByValueTest.cl 2>> output_of_time

	@echo "--- array3.cl ---"
	@echo "--- array3.cl ---" >> output_of_time
	/usr/bin/time ./clover code/array3.cl 2>> output_of_time

	@echo "--- array4.cl ---"
	@echo "--- array4.cl ---" >> output_of_time
	/usr/bin/time ./clover code/array4.cl 2>> output_of_time

	@echo "--- array5.cl ---"
	@echo "--- array5.cl ---" >> output_of_time
	/usr/bin/time ./clover code/array5.cl 2>> output_of_time

	@echo "--- array6.cl ---"
	@echo "--- array6.cl ---" >> output_of_time
	/usr/bin/time ./clover code/array6.cl 2>> output_of_time

	@echo "--- VariableArguments.cl ---"
	@echo "--- VariableArguments.cl ---" >> output_of_time
	/usr/bin/time ./clover code/VariableArguments.cl 2>> output_of_time

	@echo "--- class_method.cl ---"
	@echo "--- class_method.cl ---" >> output_of_time
	/usr/bin/time ./clover code/class_method.cl 2>> output_of_time

	@echo "--- class_method_and_field.cl ---"
	@echo "--- class_method_and_field.cl ---" >> output_of_time
	/usr/bin/time ./clover code/class_method_and_field.cl 2>> output_of_time

	@echo "--- tuple1.cl ---"
	@echo "--- tuple1.cl ---" >> output_of_time
	/usr/bin/time ./clover code/tuple1.cl 2>> output_of_time

	@echo "--- class_object1.cl ---"
	@echo "--- class_object1.cl ---" >> output_of_time
	/usr/bin/time ./clover code/class_object1.cl 2>> output_of_time

	@echo "--- class_object2.cl ---"
	@echo "--- class_object2.cl ---" >> output_of_time
	/usr/bin/time ./clover code/class_object2.cl 2>> output_of_time

	@echo "--- reflection1.cl ---"
	@echo "--- reflection1.cl ---" >> output_of_time
	/usr/bin/time ./clover code/reflection1.cl 2>> output_of_time

	@echo "--- enum.cl ---"
	@echo "--- enum.cl ---" >> output_of_time
	/usr/bin/time ./clover code/enum.cl 2>> output_of_time

	@echo "--- oror_andand.cl ---"
	@echo "--- oror_andand.cl ---" >> output_of_time
	/usr/bin/time ./clover code/oror_andand.cl 2>> output_of_time

	@echo "--- field_test1.cl ---"
	@echo "--- field_test1.cl ---" >> output_of_time
	/usr/bin/time ./clover code/field_test1.cl 2>> output_of_time

	@echo "--- field_test2.cl ---"
	@echo "--- field_test2.cl ---" >> output_of_time
	/usr/bin/time ./clover code/field_test2.cl 2>> output_of_time

	@echo "--- multiple_assinment.cl ---"
	@echo "--- multiple_assinment.cl ---" >> output_of_time
	/usr/bin/time ./clover code/multiple_assinment.cl 2>> output_of_time

	@echo "--- method_block_break.cl ---"
	@echo "--- method_block_break.cl ---" >> output_of_time
	/usr/bin/time ./clover code/method_block_break.cl 2>> output_of_time

	@echo "--- string_range.cl ---"
	@echo "--- string_range.cl ---" >> output_of_time
	/usr/bin/time ./clover code/string_range.cl 2>> output_of_time

	@echo "--- string_test3.cl ---"
	@echo "--- string_test3.cl ---" >> output_of_time
	/usr/bin/time ./clover code/string_test3.cl 2>> output_of_time

	@echo "--- method_missing.cl ---"
	@echo "--- method_missing.cl ---" >> output_of_time
	/usr/bin/time ./clover code/method_missing.cl 2>> output_of_time

	@echo "--- enum2.cl ---"
	@echo "--- enum2.cl ---" >> output_of_time
	/usr/bin/time ./clover code/enum2.cl 2>> output_of_time

	@echo "--- class_name_test.cl ---"
	@echo "--- class_name_test.cl ---" >> output_of_time
	/usr/bin/time ./clover code/class_name_test.cl 2>> output_of_time

	@echo "--- enum3.cl ---"
	@echo "--- enum3.cl ---" >> output_of_time
	/usr/bin/time ./clover code/enum3.cl 2>> output_of_time

	@echo "--- enum4.cl ---"
	@echo "--- enum4.cl ---" >> output_of_time
	/usr/bin/time ./clover code/enum4.cl 2>> output_of_time

	@echo "--- enum5.cl ---"
	@echo "--- enum5.cl ---" >> output_of_time
	/usr/bin/time ./clover code/enum5.cl 2>> output_of_time

	@echo "--- open_test.cl ---"
	@echo "--- open_test.cl ---" >> output_of_time
	/usr/bin/time ./clover code/open_test.cl 2>> output_of_time

	@echo "--- duck_typing_test.cl ---"
	@echo "--- duck_typing_test.cl ---" >> output_of_time
	/usr/bin/time ./clover code/duck_typing_test.cl 2>> output_of_time

	@echo "--- command_test.cl ---"
	@echo "--- command_test.cl ---" >> output_of_time
	/usr/bin/time ./clover code/command_test.cl 2>> output_of_time

	@echo "--- command_test2.cl ---"
	@echo "--- command_test2.cl ---" >> output_of_time
	/usr/bin/time ./clover code/command_test2.cl 2>> output_of_time

	@echo "--- native_class.cl ---"
	@echo "--- native_class.cl ---" >> output_of_time
	/usr/bin/time ./clover code/native_class.cl 2>> output_of_time

	@echo "--- preprocessor.cl ---"
	@echo "--- preprocessor.cl ---" >> output_of_time
	/usr/bin/time ./clover code/preprocessor.cl 2>> output_of_time

	@echo "--- preprocessor2.cl ---"
	@echo "--- preprocessor2.cl ---" >> output_of_time
	/usr/bin/time ./clover code/preprocessor2.cl 2>> output_of_time

	@echo "--- number.cl ---"
	@echo "--- number.cl ---" >> output_of_time
	/usr/bin/time ./clover code/number.cl 2>> output_of_time

	@echo "--- char_test.cl ---"
	@echo "--- char_test.cl ---" >> output_of_time
	/usr/bin/time ./clover code/char_test.cl 2>> output_of_time

	@echo "--- double_test.cl ---"
	@echo "--- double_test.cl ---" >> output_of_time
	/usr/bin/time ./clover code/double_test.cl 2>> output_of_time

	@echo "--- mixin_test3.cl ---"
	@echo "--- mixin_test3.cl ---" >> output_of_time
	/usr/bin/time ./clover code/mixin_test3.cl 2>> output_of_time

	@echo "--- printf.cl ---"
	@echo "--- printf.cl ---" >> output_of_time
	/usr/bin/time ./clover code/printf.cl 2>> output_of_time

	@echo "--- time.cl ---"
	@echo "--- time.cl ---" >> output_of_time
	/usr/bin/time ./clover code/time.cl 2>> output_of_time

	@echo "--- regex.cl ---"
	@echo "--- regex.cl ---" >> output_of_time
	/usr/bin/time ./clover code/regex.cl 2>> output_of_time

	@echo "--- file2.cl ---"
	@echo "--- file2.cl ---" >> output_of_time
	/usr/bin/time ./clover code/file2.cl 2>> output_of_time

	@echo "--- directory.cl ---"
	@echo "--- directory.cl ---" >> output_of_time
	/usr/bin/time ./clover code/directory.cl 2>> output_of_time

	@echo "--- termios.cl ---"
	@echo "--- termios.cl ---" >> output_of_time
	/usr/bin/time ./clover code/termios.cl 2>> output_of_time

	@echo "--- generics_test_a.cl ---"
	@echo "--- generics_test_a.cl ---" >> output_of_time
	/usr/bin/time ./clover code/generics_test_a.cl 2>> output_of_time

	@echo "--- argv_test.cl ---"
	@echo "--- argv_test.cl ---" >> output_of_time
	/usr/bin/time clover code/argv_test.cl 2>> output_of_time

	@echo "--- setenv.cl ---"
	@echo "--- setenv.cl ---" >> output_of_time
	/usr/bin/time clover code/setenv.cl 2>> output_of_time

	@echo "--- realpath.cl ---"
	@echo "--- realpath.cl ---" >> output_of_time
	/usr/bin/time clover code/realpath.cl 2>> output_of_time

	@echo "--- umask.cl ---"
	@echo "--- umask.cl ---" >> output_of_time
	/usr/bin/time clover code/umask.cl 2>> output_of_time

	@echo "--- id.cl ---"
	@echo "--- id.cl ---" >> output_of_time
	/usr/bin/time clover code/id.cl 2>> output_of_time

	@echo "--- object_field1.cl ---"
	@echo "--- object_field1.cl ---" >> output_of_time
	/usr/bin/time clover code/object_field1.cl 2>> output_of_time

	@echo "--- object_field2.cl ---"
	@echo "--- object_field2.cl ---" >> output_of_time
	/usr/bin/time clover code/object_field2.cl 2>> output_of_time

	@echo "--- instanceof2.cl ---"
	@echo "--- instanceof2.cl ---" >> output_of_time
	/usr/bin/time clover code/instanceof2.cl 2>> output_of_time

	@echo "--- is_child_test.cl ---"
	@echo "--- is_child_test.cl ---" >> output_of_time
	/usr/bin/time clover code/is_child_test.cl 2>> output_of_time

	@echo "--- class_object3.cl ---"
	@echo "--- class_object3.cl ---" >> output_of_time
	/usr/bin/time clover code/class_object3.cl 2>> output_of_time

	@echo "--- class_object4.cl ---"
	@echo "--- class_object4.cl ---" >> output_of_time
	/usr/bin/time clover code/class_object4.cl 2>> output_of_time

	@echo "--- class_object5.cl ---"
	@echo "--- class_object5.cl ---" >> output_of_time
	/usr/bin/time clover code/class_object5.cl 2>> output_of_time

	@echo "--- field_test3.cl ---"
	@echo "--- field_test3.cl ---" >> output_of_time
	/usr/bin/time clover code/field_test3.cl 2>> output_of_time

	@echo "--- field_test4.cl ---"
	@echo "--- field_test4.cl ---" >> output_of_time
	/usr/bin/time clover code/field_test4.cl 2>> output_of_time

	@echo "--- method_test.cl ---"
	@echo "--- method_test.cl ---" >> output_of_time
	/usr/bin/time clover code/method_test.cl 2>> output_of_time

	@echo "--- method_test2.cl ---"
	@echo "--- method_test2.cl ---" >> output_of_time
	/usr/bin/time clover code/method_test2.cl 2>> output_of_time

	@echo "--- type3.cl ---"
	@echo "--- type3.cl ---" >> output_of_time
	/usr/bin/time clover code/type3.cl 2>> output_of_time

	@echo "--- constructor_test.cl ---"
	@echo "--- constructor_test.cl ---" >> output_of_time
	/usr/bin/time clover code/constructor_test.cl 2>> output_of_time

	@echo "--- constructor_test2.cl ---"
	@echo "--- constructor_test2.cl ---" >> output_of_time
	/usr/bin/time clover code/constructor_test2.cl 2>> output_of_time

	@echo "--- parser.cl ---"
	@echo "--- parser.cl ---" >> output_of_time
	/usr/bin/time clover code/parser.cl 2>> output_of_time

	@echo "--- string_buffer.cl ---"
	@echo "--- string_buffer.cl ---" >> output_of_time
	/usr/bin/time clover code/string_buffer.cl 2>> output_of_time

	@echo "--- caller_test.cl ---"
	@echo "--- caller_test.cl ---" >> output_of_time
	/usr/bin/time clover code/caller_test.cl 2>> output_of_time

	@echo "--- fundamental_class_test_code.cl ---"
	@echo "--- fundamental_class_test_code.cl ---" >> output_of_time
	/usr/bin/time clover code/fundamental_class_test_code.cl 2>> output_of_time

	@echo "--- stdlib_test.cl ---"
	@echo "--- stdlib_test.cl ---" >> output_of_time
	/usr/bin/time clover code/stdlib_test.cl 2>> output_of_time

test2-body:
	@echo "Start to test2 and running code..."
	@echo "--- my_int.cl ---"
	@echo "--- my_int.cl ---" >> output_of_time
	/usr/bin/time ./clover code2/my_int.cl 2>> output_of_time

	@echo "--- my_array.cl ---"
	@echo "--- my_array.cl ---" >> output_of_time
	/usr/bin/time ./clover code2/my_array.cl 2>> output_of_time

mini-test:
	./cclover code/operand.cl

	@echo "--- operand ---"
	@echo "--- operand ---" >> output_of_time
	/usr/bin/time ./clover code/operand.cl 2>> output_of_time

########################################################
# clean
########################################################
clean:
	rm -fR clover clover.dSYM cclover cclover.dSYM iclover iclover.dSYM llclover llclover.dSYM pclover pclover.dSYM psclover psclover.dSYM src/*.o libclover* clover.exe* config.log config.status *.stackdump autom4te.cache .DS_Store *.clo code/*.class code/*.exe debug.log code/*.o *.o vm*.log *.clm core.* StandardLibrary.clc code/preprocessor.cl code/preprocessor2.cl code2/*.o code2/*.clo

distclean:
	rm -fR a.out clover clover.dSYM cclover cclover.dSYM iclover iclover.dSYM llclover llclover.dSYM pclover pclover.dSYM psclover psclover.dSYM src/*.o libclover* config.h Makefile clover.exe* config.log config.status *.stackdump autom4te.cache .DS_Store *.clo code/*.class code/*.exe debug.log code/*.o *.o vm*.log output_of_time* *.clm core.* StandardLibrary.clc code/preprocessor.cl code/preprocessor2.cl code2/*.o code2/*.clo

