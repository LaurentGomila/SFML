file(READ "${OPUS_DIR}/CMakeLists.txt" OPUS_CMAKELISTS_CONTENTS)
string(REPLACE "feature_summary(WHAT ALL)" "" OPUS_CMAKELISTS_CONTENTS "${OPUS_CMAKELISTS_CONTENTS}")
string(REPLACE "\n\nadd_library(opus" "\nset(CMAKE_DEBUG_POSTFIX d)\nadd_library(opus" OPUS_CMAKELISTS_CONTENTS "${OPUS_CMAKELISTS_CONTENTS}")
string(REPLACE "PUBLIC_HEADER\n                                 \"\${Opus_PUBLIC_HEADER}\"" "" OPUS_CMAKELISTS_CONTENTS "${OPUS_CMAKELISTS_CONTENTS}")
file(WRITE "${OPUS_DIR}/CMakeLists.txt" "${OPUS_CMAKELISTS_CONTENTS}")
