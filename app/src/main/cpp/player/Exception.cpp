//
// Created by linlin zhao on 2024/5/23.
//

#include "Exception.h"
#include "Log.h"
extern "C" {
#include "libavutil/error.h"
}

next::DecoderException::DecoderException(int error):mError(error) {
    next_log("DecoderException %s", av_err2str(error));
}
