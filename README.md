# audio.opus

This repository contains an R package which is a wrapper around the [opus-tools](https://github.com/xiph/opus-tools/) software, allowing to Encode, Inspect and Decode Audio in the 'Opus' format

The package was created with as main goal to convert opus files to wav files to be able to do automatic transcription using [audio.whisper](https://github.com/bnosac/audio.whisper). It contains

- functions to to convert opus files to wav
- which are many times faster then using ffmpeg with av

### Installation

- The package depends on libopus and libogg
    - On Windows you can install these with RTools: `pacman -S mingw-w64-x86_64-opus mingw-w64-x86_64-libogg`
    - On Mac: `brew install opus libogg libopusenc`
    - On Ubuntu `apt install libopus-dev libogg-dev`
- The package is currently not on CRAN
- For the *development* version of this package: `remotes::install_github("bnosac/audio.opus")`

Look to the documentation of the functions: `help(package = "audio.opus")`

## Example

```{r}
library(audio.opus)
file <- system.file(package = "audio.opus", "extdata", "test_opus.opus")
opus_decode(input = file, "test_opus.wav")
```


## Support in text mining

Need support in text mining?
Contact BNOSAC: http://www.bnosac.be

