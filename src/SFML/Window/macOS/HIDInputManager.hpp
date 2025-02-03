////////////////////////////////////////////////////////////
//
// SFML - Simple and Fast Multimedia Library
// Copyright (C) 2007-2025 Marco Antognini (antognini.marco@gmail.com),
//                         Laurent Gomila (laurent@sfml-dev.org)
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

#pragma once

////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include <SFML/Window/JoystickImpl.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/macOS/Utils.hpp>

#include <IOKit/hid/IOHIDDevice.h>
#include <IOKit/hid/IOHIDManager.h>
#include <vector>

#include <Carbon/Carbon.h>

namespace sf::priv
{

using IOHIDElements = std::vector<CFPtr<IOHIDElementRef>>;

////////////////////////////////////////////////////////////
/// \brief sf::priv::InputImpl helper
///
/// This class manage as a singleton instance the keyboard state.
/// Its purpose is to help sf::priv::InputImpl class.
///
////////////////////////////////////////////////////////////
class HIDInputManager
{
public:
    ////////////////////////////////////////////////////////////
    /// \brief Deleted copy constructor
    ///
    ////////////////////////////////////////////////////////////
    HIDInputManager(const HIDInputManager&) = delete;

    ////////////////////////////////////////////////////////////
    /// \brief Deleted copy assignment
    ///
    ////////////////////////////////////////////////////////////
    HIDInputManager& operator=(const HIDInputManager&) = delete;

    ////////////////////////////////////////////////////////////
    /// \brief Get the unique instance of the class
    ///
    /// \note Private use only
    ///
    /// \return Reference to the HIDInputManager instance
    ///
    ////////////////////////////////////////////////////////////
    static HIDInputManager& getInstance();

    ////////////////////////////////////////////////////////////
    /// \brief Get the usb location ID of a given device
    ///
    /// This location ID is unique and can be used a usb port identifier
    ///
    /// \param device HID device to query
    /// \return the device's location ID or 0 if something went wrong
    ///
    ////////////////////////////////////////////////////////////
    static long getLocationID(IOHIDDeviceRef device);

    ////////////////////////////////////////////////////////////
    /// \brief Create a mask (dictionary) for an IOHIDManager
    ///
    /// \param page  HID page
    /// \param usage HID usage page
    /// \return a retained CFDictionaryRef
    ///
    ////////////////////////////////////////////////////////////
    static CFPtr<CFDictionaryRef> copyDevicesMask(std::uint32_t page, std::uint32_t usage);

    ////////////////////////////////////////////////////////////
    /// \brief Try to convert a character into a SFML key code
    ///
    /// Return `sf::Keyboard::Key::Unknown` if it doesn't match any 'localized' keys.
    ///
    /// By 'localized' we mean keys that depend on the keyboard layout
    /// and might not be the same as the US keycode for some countries
    /// (e.g. the keys 'Y' and 'Z' are swapped on QWERTZ keyboard and
    /// US keyboard layouts.)
    ///
    ////////////////////////////////////////////////////////////
    static Keyboard::Key localizedKey(char16_t ch);

    ////////////////////////////////////////////////////////////
    /// \brief Opposite transformation as localizedKeys
    ///
    /// Return 0x00 (NULL) for non-convertible keys/numpad numbers.
    /// For letters, uppercase codes are returned.
    /// Some returned value are specific to macOS.
    ///
    ////////////////////////////////////////////////////////////
    static char16_t toUnicode(Keyboard::Key key);

    ////////////////////////////////////////////////////////////
    /// \brief Try to convert a virtual keycode (HID level) into a
    /// SFML scancode.
    ///
    /// Return `sf::Keyboard::Scan::Unknown` if the keycode is unknown.
    ///
    ////////////////////////////////////////////////////////////
    static Keyboard::Scancode nonLocalizedKey(char16_t virtualKeycode);

    ////////////////////////////////////////////////////////////
    /// \copydoc sf::Keyboard::isKeyPressed(Key)
    ///
    ////////////////////////////////////////////////////////////
    bool isKeyPressed(Keyboard::Key key);

    ////////////////////////////////////////////////////////////
    /// \copydoc sf::Keyboard::isKeyPressed(Scancode)
    ///
    ////////////////////////////////////////////////////////////
    bool isKeyPressed(Keyboard::Scancode code);

    ////////////////////////////////////////////////////////////
    /// \copydoc sf::Keyboard::localize
    ///
    ////////////////////////////////////////////////////////////
    Keyboard::Key localize(Keyboard::Scancode code);

    ////////////////////////////////////////////////////////////
    /// \copydoc sf::Keyboard::delocalize
    ///
    ////////////////////////////////////////////////////////////
    Keyboard::Scancode delocalize(Keyboard::Key key);

    ////////////////////////////////////////////////////////////
    /// \copydoc sf::Keyboard::getDescription
    ///
    ////////////////////////////////////////////////////////////
    String getDescription(Keyboard::Scancode code);

private:
    ////////////////////////////////////////////////////////////
    /// \brief Default constructor
    ///
    ////////////////////////////////////////////////////////////
    HIDInputManager();

    ////////////////////////////////////////////////////////////
    /// \brief Destructor
    ///
    ////////////////////////////////////////////////////////////
    ~HIDInputManager();

    ////////////////////////////////////////////////////////////
    /// \brief Initialize the keyboard part of this class
    ///
    /// If something went wrong freeUp is called.
    ///
    /// In a nutshell, this function does this:
    ///
    ///     for each connected keyboard kb:
    ///       for each key k of kb:
    ///         memorize k -> scancode mapping
    ///
    ////////////////////////////////////////////////////////////
    void initializeKeyboard();

    ////////////////////////////////////////////////////////////
    /// \brief Load the given keyboard into m_keys
    ///
    /// If the given keyboard has no key this function simply
    /// returns without calling freeUp because this is not fatal.
    ///
    /// \param keyboard Keyboard to load
    ///
    /// \see initializeKeyboard
    ///
    ////////////////////////////////////////////////////////////
    void loadKeyboard(IOHIDDeviceRef keyboard);

    ////////////////////////////////////////////////////////////
    /// \brief Load the given key into m_keys
    ///
    /// On error, freeUp is _not_ called by this function.
    ///
    /// \param key Key to load
    ///
    /// \see initializeKeyboard
    ///
    ////////////////////////////////////////////////////////////
    void loadKey(IOHIDElementRef key);

    ////////////////////////////////////////////////////////////
    /// \brief Regenerate the mappings from/to Key and Scancode
    ///
    ////////////////////////////////////////////////////////////
    void buildMappings();

    ////////////////////////////////////////////////////////////
    /// \brief Callback to regenerate mappings from/to Key and
    /// Scancode when there's a keyboard layout change
    ///
    ////////////////////////////////////////////////////////////
    static void keyboardChanged(CFNotificationCenterRef /* center */,
                                void* observer,
                                CFStringRef /* name */,
                                const void* /* object */,
                                CFDictionaryRef /* userInfo */);

    ////////////////////////////////////////////////////////////
    /// \brief Release all resources
    ///
    /// Close all connections to any devices.
    ///
    /// \see initializeKeyboard
    ///
    ////////////////////////////////////////////////////////////
    void freeUp();

    ////////////////////////////////////////////////////////////
    /// \brief Filter the devices and return them
    ///
    /// On error, freeUp is _not_ called by this function.
    ///
    /// \param page  HID page like kHIDPage_GenericDesktop
    /// \param usage HID usage page like kHIDUsage_GD_Keyboard or kHIDUsage_GD_Mouse
    /// \return a retained, non-empty __CFSet pointer of IOHIDDeviceRef or a null pointer
    ///
    ////////////////////////////////////////////////////////////
    CFPtr<CFSetRef> copyDevices(std::uint32_t page, std::uint32_t usage);

    ////////////////////////////////////////////////////////////
    /// \brief Check if a key is pressed
    ///
    /// \param elements HID elements mapping to this key
    ///
    /// \return `true` if the key is pressed, `false` otherwise
    ///
    /// \see isKeyPressed, isMouseButtonPressed
    ///
    ////////////////////////////////////////////////////////////
    bool isPressed(IOHIDElements& elements) const;

    ////////////////////////////////////////////////////////////
    /// \brief Convert a HID key usage to its corresponding scancode
    ///
    /// \param usage Any kHIDUsage_Keyboard* usage
    /// \return the scancode associated with the given HID key usage
    ///         or Scan::Unknown if it is associated with no scancode.
    ///
    ////////////////////////////////////////////////////////////
    static Keyboard::Scancode usageToScancode(std::uint32_t usage);

    ////////////////////////////////////////////////////////////
    /// Convert the scancode to the expected virtual code.
    ///
    ////////////////////////////////////////////////////////////
    static std::uint8_t scanToVirtualCode(Keyboard::Scancode code);

    ////////////////////////////////////////////////////////////
    /// Fallback conversion for keys which aren't expected to be impacted
    /// by the layout. Can return Unknown.
    ///
    ////////////////////////////////////////////////////////////
    static Keyboard::Key localizedKeyFallback(Keyboard::Scancode code);

    ////////////////////////////////////////////////////////////
    // Member data
    ////////////////////////////////////////////////////////////
    CFPtr<IOHIDManagerRef> m_manager;           ///< Underlying HID Manager
    bool                   m_keysInitialized{}; ///< Has initializeKeyboard been called at least once?
    EnumArray<Keyboard::Scancode, IOHIDElements, Keyboard::ScancodeCount> m_keys; ///< All the keys on any connected keyboard
    EnumArray<Keyboard::Key, Keyboard::Scancode, Keyboard::KeyCount> m_keyToScancodeMapping{}; ///< Mapping from Key to Scancode
    EnumArray<Keyboard::Scancode, Keyboard::Key, Keyboard::ScancodeCount> m_scancodeToKeyMapping{}; ///< Mapping from Scancode to Key

    ////////////////////////////////////////////////////////////
    /// m_keys' index corresponds to sf::Keyboard::Scancode enum.
    /// If no key is assigned with key XYZ then m_keys[XYZ].size() == 0.
    /// If there are several keyboards connected and several HID keys associated
    /// with the same sf::Keyboard::Key then m_keys[XYZ] contains all these
    /// HID keys.
    ///
    /// The mappings (both directions) get invalidated when the
    /// keyboard layout changes. They both default to (Scan::)Unknown.
    ///
    ////////////////////////////////////////////////////////////
};

} // namespace sf::priv
