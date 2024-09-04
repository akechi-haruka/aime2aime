aime2aime
segatools aimeio DLL to convert from Aime card readers to ... Aime card readers!
2024 Haruka
Licensed under the GPLv3.

--- Wait what? ---

This allows to intercept communication between the game and a real physical Aime reader. It adds support for the segatools API (namely, packets 25, 26, 31 and 33. This allows you to use something like eMoneyUILink to add eMoney support to older games that do not have eMoney support. And possibly other things.

--- Usage ---

* Place aime2aime.dll and aime2aime.ini in your game directory.
* in your segatools.ini, set this value:
[aimeio]
path=aime2aime.dll
