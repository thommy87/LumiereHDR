![Logo](images/luminance.svg)
# LumièreHDR
---------------
[![CI](https://github.com/thommy87/LumiereHDR/actions/workflows/ci.yml/badge.svg)](https://github.com/thommy87/LumiereHDR/actions/workflows/ci.yml)
[![License](https://img.shields.io/github/license/thommy87/LumiereHDR.svg)](LICENSE)

LumièreHDR is a modernized fork of [Luminance HDR](https://github.com/LuminanceHDR/LuminanceHDR)
2.6, focused on security, a new user interface and current platforms.
It stays free software under the GNU GPL v2.

- Releases: https://github.com/thommy87/LumiereHDR/releases
- [Code signing policy](CODE_SIGNING_POLICY.md): free code signing provided by [SignPath.io](https://about.signpath.io), certificate by [SignPath Foundation](https://signpath.org)
- [Privacy policy](PRIVACY.md)

Luminance HDR, the project this fork is based on:

Copyright (C) 2010-2019

 - Davide Anastasia <davideanastasia@users.sourceforge.net>
 - Franco Comida <fcomida@users.sourceforge.net>
 - Daniel Kaneider <danielkaneider@users.sourceforge.net>

 and many contributors and translators

Copyright (C) 2006-2010 - Giuseppe Rota <grota@users.sourceforge.net>

- Webpage: http://qtpfsgui.sourceforge.net
- Sourceforge: http://sourceforge.net/projects/qtpfsgui
- Github: https://github.com/LuminanceHDR/LuminanceHDR

What it is
----------
Luminance HDR is a graphical user interface (based on the Qt5 toolkit) that provides a complete workflow for HDR imaging.

Supported HDR formats:
* OpenEXR (extension: exr)
* Radiance RGBE (extension: hdr)
* Tiff formats: 16bit, 32bit (float) and LogLuv (extension: tiff)
* Raw image formats (extension: various)
* PFS native format (extension: pfs)

Supported LDR formats:
* JPEG, PNG, PPM, PBM, TIFF, FITS

Supported features:
* Create an HDR file from a set of images (JPEG, TIFF 8bit and 16bit, RAW)
of the same scene taken at different exposure setting
* Save and load HDR files
* Rotate and resize HDR files
* Tonemap HDR images
* Projective Transformations
* Copy EXIF data between sets of images
* Supports internationalization

Raw image formats are supported - and treated as HDR - thanks to [LibRAW](http://www.libraw.org/).

Make sure you read the "Dependencies" Section in the [INSTALL.md](INSTALL.md) file. If you intend to make a package for a GNU/Linux distribution, please refer to the same file for more information.

The code is in part based on the existing open source packages:
- "pfstools", "pfstmo" and "pfscalibration" by Grzegorz Krawczyk and Rafal Mantiuk
- "qpfstmo", by Nicholas Phillips.

Without their contribution all of this would have not been possible.

Dependencies
------------
Please, refer to the [INSTALL.md](INSTALL.md) file

Compiling the sources
---------------------
Please, refer to the [INSTALL.md](INSTALL.md) file

Contact and Links
-----------------
All comments and suggestions concerning this package or the implementation of particular algorithms are welcome.

For bugs, issues or feature requests, please refer to the [BUGS](BUGS) file

See also:
* [PFStools](http://www.mpii.mpg.de/resources/pfstools/)
* [PFStmo (a tone mapping library)](http://www.mpii.mpg.de/resources/tmo/)
* [PFScalibration](http://www.mpii.mpg.de/resources/hdr/calibration/pfs.html)
