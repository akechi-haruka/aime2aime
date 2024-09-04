cd %~dp0
chcp 65001
C:\msys64\usr\bin\bash --login -c "DIR=%cd:\=\\\\%; cd $(cygpath $DIR) && ./compile.sh"

copy _build32\aime2aime\aime2aime.dll W:\apm\BZ10\aime2aime.dll