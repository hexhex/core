gsed -i.orig -e '/-Wl,-R/ { s/-Wl,-R/-Wl,-rpath,/; }' configure
gsed -i.orig -e 's/-lclasp/${CLASP_TRUNK_DIR}\/build\/release\/libclasp\/*.o/' configure
gsed -i.orig -e 's/-lappgringo -lgringo/${GRINGO_TRUNK_DIR}\/build\/release\/libprogram_opts\/CMakeFiles\/program_opts-lib.dir\/src\/*.o ${GRINGO_TRUNK_DIR}\/build\/release\/libgringo\/CMakeFiles\/gringo-lib.dir\/src\/*.o ${GRINGO_TRUNK_DIR}\/build\/release\/libgringo\/CMakeFiles\/gringo-lib.dir\/src\/output\/*.o/' configure
bash configure $@
