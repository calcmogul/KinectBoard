////////////////////////////////////////////////////////////
//
// SFML - Simple and Fast Multimedia Library
// Copyright (C) 2007-2012 Laurent Gomila (laurent.gom@gmail.com)
//
// This software is provided 'as-is', without any express or implied warranty.
// In no event will the authors be held liable for any damages arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it freely,
// subject to the following restrictions:
//
// 1. The origin of this software must not be misrepresented;
//    you must not claim that you wrote the original software.
//    If you use this software in a product, an acknowledgment
//    in the product documentation would be appreciated but is not required.
//
// 2. Altered source versions must be plainly marked as such,
//    and must not be misrepresented as being the original software.
//
// 3. This notice may not be removed or altered from any source distribution.
//
////////////////////////////////////////////////////////////

/* !!! THIS IS AN EXTREMELY ALTERED AND PURPOSE-BUILT VERSION OF SFML !!!
 * This distribution is designed to possess only a limited subset of the
 * original library's functionality and to only build on Win32 platforms.
 * The original distribution of this software has many more features and
 * supports more platforms.
 */

#ifndef SFML_TIME_HPP
#define SFML_TIME_HPP

////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "../Config.hpp"


namespace sf
{
////////////////////////////////////////////////////////////
/// \brief Represents a time value
///
////////////////////////////////////////////////////////////
class Time
{
public :

    ////////////////////////////////////////////////////////////
    /// \brief Default constructor
    ///
    /// Sets the time value to zero.
    ///
    ////////////////////////////////////////////////////////////
    Time();

    ////////////////////////////////////////////////////////////
    /// \brief Return the time value as a number of seconds
    ///
    /// \return Time in seconds
    ///
    /// \see asMilliseconds, asMicroseconds
    ///
    ////////////////////////////////////////////////////////////
    float asSeconds() const;

    ////////////////////////////////////////////////////////////
    /// \brief Return the time value as a number of microseconds
    ///
    /// \return Time in microseconds
    ///
    /// \see asSeconds, asMilliseconds
    ///
    ////////////////////////////////////////////////////////////
    int64_t asMicroseconds() const;

    ////////////////////////////////////////////////////////////
    // Static member data
    ////////////////////////////////////////////////////////////
    static const Time Zero; ///< Predefined "zero" time value 

private :

    friend Time microseconds(int64_t);

    ////////////////////////////////////////////////////////////
    /// \brief Construct from a number of microseconds
    ///
    /// This function is internal. To construct time values,
    /// use sf::seconds, sf::milliseconds or sf::microseconds instead.
    ///
    /// \param microseconds Number of microseconds
    ///
    ////////////////////////////////////////////////////////////
    explicit Time(int64_t microseconds);

private :

    ////////////////////////////////////////////////////////////
    // Member data
    ////////////////////////////////////////////////////////////
    int64_t m_microseconds; ///< Time value stored as microseconds
};

////////////////////////////////////////////////////////////
/// \relates Time
/// \brief Construct a time value from a number of microseconds
///
/// \param amount Number of microseconds
///
/// \return Time value constructed from the amount of microseconds
///
/// \see seconds, milliseconds
///
////////////////////////////////////////////////////////////
Time microseconds(int64_t amount);

////////////////////////////////////////////////////////////
/// \relates Time
/// \brief Overload of binary - operator to subtract two time values
///
/// \param left  Left operand (a time)
/// \param right Right operand (a time)
///
/// \return Difference of the two times values
///
////////////////////////////////////////////////////////////
Time operator -(Time left, Time right);

} // namespace sf


#endif // SFML_TIME_HPP


////////////////////////////////////////////////////////////
/// \class sf::Time
/// \ingroup system
///
/// sf::Time encapsulates a time value in a flexible way.
/// It allows to define a time value as a number of
/// microseconds. It also works the other way around: you can
/// read a time value as either a number of seconds or
/// microseconds.
///
/// You can subtract two times
///
/// Since they represent a time span and not an absolute time
/// value, times can also be negative.
///
/// Usage example:
/// \code
/// sf::Time t1 = sf::seconds(0.1f);
/// Int32 micro = t1.asMicroseconds(); // 100000
///
/// sf::Time t2 = sf::microseconds(-800000);
/// float sec = t2.asSeconds(); // -0.8
/// \endcode
///
/// \see sf::Clock
///
////////////////////////////////////////////////////////////
