/******************************************************************************
*   Copyright (C) 2007 by Aaron Seigo <aseigo@kde.org>                        *
*                                                                             *
*   This library is free software; you can redistribute it and/or             *
*   modify it under the terms of the GNU Library General Public               *
*   License as published by the Free Software Foundation; either              *
*   version 2 of the License, or (at your option) any later version.          *
*                                                                             *
*   This library is distributed in the hope that it will be useful,           *
*   but WITHOUT ANY WARRANTY; without even the implied warranty of            *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU          *
*   Library General Public License for more details.                          *
*                                                                             *
*   You should have received a copy of the GNU Library General Public License *
*   along with this library; see the file COPYING.LIB.  If not, write to      *
*   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,      *
*   Boston, MA 02110-1301, USA.                                               *
*******************************************************************************/

namespace Plasma
{

/**
 * @brief object representing an installed Plasmagik package
 **/

class PackageStructure;

class Package
{
    public:
        /**
         * Default constructor
         *
         * @arg packageRoot path to the package installation root
         * @arg package the name of the package
         * @arg structure the package structure describing this package
         **/
        Package(const QString& packageRoot, const QString& package,
                const PackageStructure& structure);
        ~Package();

        /**
         * Get the path to a given file.
         *
         * @arg fileType the type of file to look for, as defined in the
         *               package structure
         * @arg filename the name of the file
         * @return path to the file on disk. QString() if not found.
         **/
        QString filePath(const char* fileType, const QString& filename);

        /**
         * Get the path to a given file.
         *
         * @arg fileType the type of file to look for, as defined in the
         *               package structure. The type must refer to a file
         *               in the package structure and not a directory.
         * @return path to the file on disk. QString() if not found
         **/
        QString filePath(const char* fileType);

        /**
         * Get the list of files of a given type.
         *
         * @arg fileType the type of file to look for, as defined in the
         *               package structure.
         * @return list of files by name, suitable for passing to filePath
         **/
        QStringList entryList(const char* fileType);

        /**
         * 
         *
         * @param packageRoot path to the directory where Plasmagik packages
         *                    have been installed to
         * @return a list of installed Plasmagik packages
         **/
        static QStringList knownPackages(const QString& packageRoot);

        /**
         * @param package path to the Plasmagik package
         * @param packageRoot path to the directory where the package should be
         *                    installed to
         * @return true on successful installation, false otherwise
         **/
        static bool installPackage(const QString& package, const QString& packageRoot);

    private:
        class Private;
        Private * const d;
};

} // Namespace
