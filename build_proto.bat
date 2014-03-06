@echo off
..\ext-deps\protobuf-2.5.0\bin\protoc.exe --cpp_out=. protobuf/data.proto
pause
