#' Decode an Opus audio file to WAV
#'
#' @param input   Path to the input \code{.opus} file, or \code{"-"} for stdin.
#' @param output  Path to the output \code{.wav} file.
#' @param rate    Integer. Target sample rate in Hz. \code{0} (default) keeps
#'                the stream's original rate (falling back to 48000 if unknown).
#' @param stereo  Logical. Force stereo output. Default \code{FALSE}.
#' @param gain_db Numeric. Manual gain adjustment in dB. Default \code{0}.
#' @param float   Logical. Write 32-bit IEEE float WAV instead of 16-bit PCM.
#'                Default \code{FALSE}.
#'
#' @return Invisibly returns \code{0} on success, \code{1} on failure.
#'
#' @examples
#' \dontrun{
#'   opus_decode("speech.opus", "speech.wav")
#'   opus_decode("music.opus",  "music.wav", rate = 44100, stereo = TRUE)
#'   opus_decode("audio.opus",  "audio.wav", gain_db = -3, float = TRUE)
#' }
#' @export
opus_decode <- function(input,
                        output,
                        rate    = 0L,
                        stereo  = FALSE,
                        gain_db = 0,
                        float   = FALSE) {
    
    stopifnot(
        is.character(input),  length(input)  == 1L,
        is.character(output), length(output) == 1L
    )
    
    rate    <- as.integer(rate)
    stereo  <- as.logical(stereo)
    gain_db <- as.double(gain_db)
    float   <- as.logical(float)
    
    result <- .Call("C_opus_decode",
                    input, output,
                    rate, stereo, gain_db, float)
    
    if (result != 0L)
        warning("opus_decode: decoding finished with error code ", result)
    
    invisible(result)
}