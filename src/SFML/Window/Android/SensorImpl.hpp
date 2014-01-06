////////////////////////////////////////////////////////////
//
// SFML - Simple and Fast Multimedia Library
// Copyright (C) 2013 Jonathan De Wachter (dewachter.jonathan@gmail.com)
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

#ifndef SFML_SENSORIMPLANDROID_HPP
#define SFML_SENSORIMPLANDROID_HPP

////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include <android/sensor.h>

namespace sf
{
namespace priv
{
////////////////////////////////////////////////////////////
/// \brief Android implementation of sensors
///
////////////////////////////////////////////////////////////
class SensorImpl
{
public :

    ////////////////////////////////////////////////////////////
    /// \brief Perform the global initialization of the sensor module
    ///
    ////////////////////////////////////////////////////////////
    static void initialize();

    ////////////////////////////////////////////////////////////
    /// \brief Perform the global cleanup of the sensor module
    ///
    ////////////////////////////////////////////////////////////
    static void cleanup();

    ////////////////////////////////////////////////////////////
    /// \brief Initialize the sensor
    ///
    /// \param type Index assigned to the sensor
    ///
    /// \return The sensor capabilities
    ///
    ////////////////////////////////////////////////////////////
    SensorCaps& initialize(unsigned int type);

    ////////////////////////////////////////////////////////////
    /// \brief Close the sensor
    ///
    ////////////////////////////////////////////////////////////
    void terminate();

    ////////////////////////////////////////////////////////////
    /// \brief Update the sensor and get its new state
    ///
    /// \return Sensor state
    ///
    ////////////////////////////////////////////////////////////
    SensorState& update();

    ////////////////////////////////////////////////////////////
    /// \brief Check if the sensor is enabled
    ///
    /// \return True if the sensor is enabled, false otherwise
    ///
    ////////////////////////////////////////////////////////////
    bool isEnable();

    ////////////////////////////////////////////////////////////
    /// \brief Enable or disable the sensor
    ///
    /// \param enable True to enable, false to disable
    ///
    ////////////////////////////////////////////////////////////
    void setEnable(bool enable);
    
    ////////////////////////////////////////////////////////////
    /// \brief Set the refresh rate of the sensor
    ///
    /// \param rate Delay between each refresh
    ///
    ////////////////////////////////////////////////////////////
    void setRefreshRate(const Time& rate);

private:    
    ////////////////////////////////////////////////////////////
    /// \brief Get the default Android sensor matching the sensor type
    ///
    /// \param type Type of the sensor
    ///
    /// \return The default Android sensor, NULL otherwise
    ///
    ////////////////////////////////////////////////////////////
    ASensor const* getDefaultSensor(unsigned int type);
    
    ////////////////////////////////////////////////////////////
    /// \brief Process the pending sensor data available and add them to our lists
    ///
    /// \param fd     File descriptor
    /// \param events Bitmask of the poll events that were triggered
    /// \param data   Data pointer supplied
    ///
    /// \return Whether it should continue (1) or unregister the callback (0)
    ///
    ////////////////////////////////////////////////////////////
    static int processSensorEvents(int fd, int events, void* data);
    
    ////////////////////////////////////////////////////////////
    // Member data
    ////////////////////////////////////////////////////////////
    const ASensor* m_sensor; ///< Android sensor structure
    unsigned int   m_index;  ///< Index of the sensor
    SensorState    m_state;  ///< Sensor state
};

} // namespace priv

} // namespace sf


#endif // SFML_SENSORIMPLANDROID_HPP
