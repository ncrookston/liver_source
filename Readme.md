Instructions for building model:

mkdir \_bld
cd \_bld
conan install .. --build
cmake .. -DCMAKE_BUILD_TYPE=Release

See ./bin for various programs.

For protobuf descriptions of the generated liver models, see liver_source/messages. Those can be used with a wide variety of programming languages to load the models.
