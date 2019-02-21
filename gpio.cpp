////////////////////////////////////////////////////////////////////////////////////////////////////////
///
/// For simple testing GPIO.
/// How to Compile: g++ -std=c++11 gpio.cpp
///
////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <memory>
#include <chrono>
#include <thread>

#define GPIO_EXPORT_PATH "/sys/class/gpio/export"
#define GPIO_DIRECTORY_PATH "/sys/class/gpio/gpio%d"
#define GPIO_DIRECTION_PATH "/sys/class/gpio/gpio%d/direction"
#define GPIO_VALUE_PATH "/sys/class/gpio/gpio%d/value"
#define GPIO_EDGE_PATH "/sys/class/gpio/gpio%d/edge"

// Set GPIO pin number that you will use
#define GPIO_NUMBER_FOR_TEST 18
#define GPIO_NUMBER_FOR_INTERRUPT 17

class GPIO
{
public:
  ////////////////////////////////////////////////////////////////////////////////////////////////////////
  /// @fn GPIO
  ///
  /// Create GPIO file using /sys/class/gpio/export. Then open direction file and value file
  ///
  /// @param Pin   a gpio pin number
  ////////////////////////////////////////////////////////////////////////////////////////////////////////
  GPIO(int pin)
    : m_pin(pin)
  {
    char FileName[64];

    snprintf(FileName, sizeof(FileName), GPIO_DIRECTORY_PATH, pin);
    if (access(FileName, F_OK) != 0)
    {
      FILE* ExportFile = fopen(GPIO_EXPORT_PATH, "w");
      if (ExportFile == NULL)
      {
        exit(EXIT_FAILURE);
      }
      fprintf(ExportFile, "%d", pin);
      fflush(ExportFile);
      fclose(ExportFile);
    }

    snprintf(FileName, sizeof(FileName), GPIO_DIRECTION_PATH, pin);
    m_gpioDirectionFile = open(FileName, O_WRONLY | O_CLOEXEC);

    snprintf(FileName, sizeof(FileName), GPIO_VALUE_PATH, pin);
    m_gpioValueFile = open(FileName, O_RDWR | O_CLOEXEC);

    if ((m_gpioDirectionFile < 0) || (m_gpioValueFile < 0))
    {
      exit(EXIT_FAILURE);
    }
  }

  ////////////////////////////////////////////////////////////////////////////////////////////////////////
  /// @fn setOutput
  ///
  /// Write high or low to direction file.
  ///
  /// @high   Set output level whether high or not
  ////////////////////////////////////////////////////////////////////////////////////////////////////////
  void setOutput(bool high)
  {
    if (high)
    {
      write(m_gpioDirectionFile, "high", 4);
    }
    else
    {
      write(m_gpioDirectionFile, "low", 3);
    }
  }

  ////////////////////////////////////////////////////////////////////////////////////////////////////////
  /// @fn getValue
  ///
  /// Get value from value file
  ///
  /// @return value is whether high or not
  ////////////////////////////////////////////////////////////////////////////////////////////////////////
  bool getValue(void)
  {
    unsigned char value;

    lseek(m_gpioValueFile, SEEK_SET, 0);
    read(m_gpioValueFile, &value, 1);

    if (value == '0')
    {
      return false;
    }

    return true;
  }

  ////////////////////////////////////////////////////////////////////////////////////////////////////////
  /// @fn setEdge
  ///
  /// Set interruption type (both) to Edge file
  ///
  /// @return Value File
  ////////////////////////////////////////////////////////////////////////////////////////////////////////
  int setEdge(void)
  {
    char FileName[64];

    snprintf(FileName, sizeof(FileName), GPIO_EDGE_PATH, m_pin);
    int EdgeFile = open(FileName, O_WRONLY);
    if (EdgeFile < 0)
    {
      exit(EXIT_FAILURE);
    }
    write(EdgeFile, "both", 4);
    close(EdgeFile);

    return m_gpioValueFile;
  }

private:
  int m_pin;
  int m_gpioDirectionFile;
  int m_gpioValueFile;
};

int main (void)
{
  // Create instances for each GPIO pin
  std::unique_ptr<GPIO> m_outputPin = std::unique_ptr<GPIO>(new GPIO(GPIO_NUMBER_FOR_TEST));
  std::unique_ptr<GPIO> m_interruptPin = std::unique_ptr<GPIO>(new GPIO(GPIO_NUMBER_FOR_INTERRUPT));

  bool currentValue = m_interruptPin->getValue();
  bool nextValue;

  // Test for 5 min
  std::chrono::system_clock::time_point start = std::chrono::system_clock::now();
  while (std::chrono::duration_cast<std::chrono::seconds>
	 ((std::chrono::system_clock::now())-start).count() < 300)
  { 
    // Set poll for interruption
    struct pollfd pfd;
    pfd.fd = m_interruptPin->setEdge();
    pfd.events = POLLPRI;

    // Polling
    if (poll(&pfd, 1, -1) <= 0)
    {
      exit(EXIT_FAILURE);
    }

    if (pfd.revents & POLLPRI)
    {
      nextValue = m_interruptPin->getValue();
      if (currentValue != nextValue)
      {
	// Wait for debounce time 50ms. This code doesn't assume continuous hits of push button.
	std::this_thread::sleep_for(std::chrono::milliseconds(50));

	if (nextValue == m_interruptPin->getValue())
	{
	  currentValue = nextValue;
	  if (nextValue)
	  {
	    // Turn LED on
	    m_outputPin->setOutput(true);
	  }
	  else
	  {
	    // Turn LED off
	    m_outputPin->setOutput(false);
	  }
	}
      }
    }
  }

  // Turn LED off at the end
  m_outputPin->setOutput(false);
}
