# audio.opus

This repository contains an R package which is a wrapper around the [opus-tools](https://github.com/xiph/opus-tools/) software, allowing to Encode, Inspect and Decode Audio in the 'Opus' format

The package was created with as main goal to convert opus files to wav files to be able to do automatic transcription using [audio.whisper](https://github.com/bnosac/audio.whisper). It contains

- functions to to convert opus files to wav
- which are many times faster then using ffmpeg with av

### Installation

- The package is currently not on CRAN
- For the *development* version of this package: `remotes::install_github("bnosac/audio.opus")`

Look to the documentation of the functions: `help(package = "audio.opus")`

## Example

```{r}
library(audio.opus)
file <- system.file(package = "audio.opuse", "extdata", "test_opus.opus")
opus_decode(input = file, "test_opus.wav")
```


## Support in text mining

Need support in text mining?
Contact BNOSAC: http://www.bnosac.be

