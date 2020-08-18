Instructions for building model:

```
mkdir _bld
cd _bld
conan install .. --build=missing
cmake .. -DCMAKE_BUILD_TYPE=Release
```
See `_bld/bin` for various programs.

For protobuf descriptions of the generated liver models, see `messages/*`. Those can be used with a wide variety of programming languages to load the models.
