/****************************************************************************
 *
 *   Copyright (C) 2012 PX4 Development Team. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name PX4 nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/**
 * @file Definitions for the generic base classes in the device framework.
 */

#ifndef _DEVICE_DEVICE_H
#define _DEVICE_DEVICE_H

/*
 * Includes here should only cover the needs of the framework definitions.
 */
#include <nuttx/config.h>

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <poll.h>

#include <nuttx/fs/fs.h>

/**
 * Namespace encapsulating all device framework classes, functions and data.
 */
namespace device __EXPORT 
{

/**
 * Fundamental base class for all device drivers.
 *
 * This class handles the basic "being a driver" things, including
 * interrupt registration and dispatch.
 */
class __EXPORT Device
{
public:
	/**
	 * Interrupt handler.
	 */
	virtual void	interrupt(void *ctx);	/**< interrupt handler */

protected:
	const char	*_name;			/**< driver name */
	bool		_debug_enabled;		/**< if true, debug messages are printed */

	/**
	 * Constructor
	 *
	 * @param name		Driver name
	 * @param irq		Interrupt assigned to the device.
	 */
	Device(const char *name,
	       int irq = 0);
	~Device();

	/**
	 * Initialise the driver and make it ready for use.
	 *
	 * @return	OK if the driver initialised OK.
	 */
	virtual int	init();

	/**
	 * Enable the device interrupt
	 */
	void		interrupt_enable();

	/**
	 * Disable the device interrupt
	 */
	void		interrupt_disable();

	/**
	 * Take the driver lock.
	 *
	 * Each driver instance has its own lock/semaphore.
	 *
	 * Note that we must loop as the wait may be interrupted by a signal.
	 */
	void		lock() {
		do {} while (sem_wait(&_lock) != 0);
	}

	/**
	 * Release the driver lock.
	 */
	void		unlock() {
		sem_post(&_lock);
	}

	/**
	 * Log a message.
	 *
	 * The message is prefixed with the driver name, and followed
	 * by a newline.
	 */
	void		log(const char *fmt, ...);

	/**
	 * Print a debug message.
	 *
	 * The message is prefixed with the driver name, and followed
	 * by a newline.
	 */
	void		debug(const char *fmt, ...);

private:
	int		_irq;
	bool		_irq_attached;
	sem_t		_lock;

	/** disable copy construction for this and all subclasses */
	Device(const Device &);

	/** disable assignment for this and all subclasses */
	Device &operator = (const Device &);

	/**
	 * Register ourselves as a handler for an interrupt
	 *
	 * @param irq		The interrupt to claim
	 * @return		OK if the interrupt was registered
	 */
	int		dev_register_interrupt(int irq);

	/**
	 * Unregister ourselves as a handler for any interrupt
	 */
	void		dev_unregister_interrupt();

	/**
	 * Interrupt dispatcher
	 *
	 * @param irq		The interrupt that has been triggered.
	 * @param context	Pointer to the interrupted context.
	 */
	static void	dev_interrupt(int irq, void *context);
};

/**
 * Abstract class for any character device
 */
class __EXPORT CDev : public Device
{
public:
	/**
	 * Constructor
	 *
	 * @param name		Driver name
	 * @param devname	Device node name
	 * @param irq		Interrupt assigned to the device
	 */
	CDev(const char *name, const char *devname, int irq = 0);

	/**
	 * Destructor
	 */
	~CDev();

	virtual int	init();

	/**
	 * Handle an open of the device.
	 *
	 * This function is called for every open of the device. The default
	 * implementation maintains _open_count and always returns OK.
	 *
	 * @param filp		Pointer to the NuttX file structure.
	 * @return		OK if the open is allowed, -errno otherwise.
	 */
	virtual int	open(struct file *filp);

	/**
	 * Handle a close of the device.
	 *
	 * This function is called for every close of the device. The default
	 * implementation maintains _open_count and returns OK as long as it is not zero.
	 *
	 * @param filp		Pointer to the NuttX file structure.
	 * @return		OK if the close was successful, -errno otherwise.
	 */
	virtual int	close(struct file *filp);

	/**
	 * Perform a read from the device.
	 *
	 * The default implementation returns -ENOSYS.
	 *
	 * @param filp		Pointer to the NuttX file structure.
	 * @param buffer	Pointer to the buffer into which data should be placed.
	 * @param buflen	The number of bytes to be read.
	 * @return		The number of bytes read or -errno otherwise.
	 */
	virtual ssize_t	read(struct file *filp, char *buffer, size_t buflen);

	/**
	 * Perform a write to the device.
	 *
	 * The default implementation returns -ENOSYS.
	 *
	 * @param filp		Pointer to the NuttX file structure.
	 * @param buffer	Pointer to the buffer from which data should be read.
	 * @param buflen	The number of bytes to be written.
	 * @return		The number of bytes written or -errno otherwise.
	 */
	virtual ssize_t	write(struct file *filp, const char *buffer, size_t buflen);

	/**
	 * Perform a logical seek operation on the device.
	 *
	 * The default implementation returns -ENOSYS.
	 *
	 * @param filp		Pointer to the NuttX file structure.
	 * @param offset	The new file position relative to whence.
	 * @param whence	SEEK_OFS, SEEK_CUR or SEEK_END.
	 * @return		The previous offset, or -errno otherwise.
	 */
	virtual off_t	seek(struct file *filp, off_t offset, int whence);

	/**
	 * Perform an ioctl operation on the device.
	 *
	 * The default implementation handles DIOC_GETPRIV, and otherwise
	 * returns -ENOTTY. Subclasses should call the default implementation
	 * for any command they do not handle themselves.
	 *
	 * @param filp		Pointer to the NuttX file structure.
	 * @param cmd		The ioctl command value.
	 * @param arg		The ioctl argument value.
	 * @return		OK on success, or -errno otherwise.
	 */
	virtual int	ioctl(struct file *filp, int cmd, unsigned long arg);

	/**
	 * Perform a poll setup/teardown operation.
	 *
	 * This is handled internally and should not normally be overridden.
	 *
	 * @param filp		Pointer to the NuttX file structure.
	 * @param fds		Poll descriptor being waited on.
	 * @param arg		True if this is establishing a request, false if
	 *			it is being torn down.
	 * @return		OK on success, or -errno otherwise.
	 */
	virtual int	poll(struct file *filp, struct pollfd *fds, bool setup);

       /**
        * Test whether the device is currently open.
        *
        * This can be used to avoid tearing down a device that is still active.
        *
        * @return              True if the device is currently open.
        */
       bool            is_open() { return _open_count > 0; }

protected:
	/**
	 * Check the current state of the device for poll events from the
	 * perspective of the file.
	 *
	 * This function is called by the default poll() implementation when
	 * a poll is set up to determine whether the poll should return immediately.
	 *
	 * The default implementation returns no events.
	 *
	 * @param filp		The file that's interested.
	 * @return		The current set of poll events.
	 */
	virtual pollevent_t poll_state(struct file *filp);

	/**
	 * Report new poll events.
	 *
	 * This function should be called anytime the state of the device changes
	 * in a fashion that might be interesting to a poll waiter.
	 *
	 * @param events	The new event(s) being announced.
	 */
	virtual void	poll_notify(pollevent_t events);

	/**
	 * Internal implementation of poll_notify.
	 *
	 * @param fds		A poll waiter to notify.
	 * @param events	The event(s) to send to the waiter.
	 */
	virtual void	poll_notify_one(struct pollfd *fds, pollevent_t events);

	/**
	 * Notification of the first open.
	 *
	 * This function is called when the device open count transitions from zero
	 * to one.  The driver lock is held for the duration of the call.
	 *
	 * The default implementation returns OK.
	 *
	 * @param filp		Pointer to the NuttX file structure.
	 * @return		OK if the open should proceed, -errno otherwise.
	 */
	virtual int	open_first(struct file *filp);

	/**
	 * Notification of the last close.
	 *
	 * This function is called when the device open count transitions from
	 * one to zero.  The driver lock is held for the duration of the call.
	 *
	 * The default implementation returns OK.
	 *
	 * @param filp		Pointer to the NuttX file structure.
	 * @return		OK if the open should return OK, -errno otherwise.
	 */
	virtual int	close_last(struct file *filp);

private:
	static const unsigned _max_pollwaiters = 8;

	const char	*_devname;		/**< device node name */
	bool		_registered;		/**< true if device name was registered */
	unsigned	_open_count;		/**< number of successful opens */

	struct pollfd	*_pollset[_max_pollwaiters];

	/**
	 * Store a pollwaiter in a slot where we can find it later.
	 *
	 * Expands the pollset as required.  Must be called with the driver locked.
	 *
	 * @return		OK, or -errno on error.
	 */
	int		store_poll_waiter(struct pollfd *fds);

	/**
	 * Remove a poll waiter.
	 *
	 * @return		OK, or -errno on error.
	 */
	int		remove_poll_waiter(struct pollfd *fds);
};

/**
 * Abstract class for character device accessed via PIO
 */
class __EXPORT PIO : public CDev
{
public:
	/**
	 * Constructor
	 *
	 * @param name		Driver name
	 * @param devname	Device node name
	 * @param base		Base address of the device PIO area
	 * @param irq		Interrupt assigned to the device (or zero if none)
	 */
	PIO(const char *name,
	    const char *devname,
	    uint32_t base,
	    int irq = 0);
	~PIO();

	int		init();

protected:

	/**
	 * Read a register
	 *
	 * @param offset	Register offset in bytes from the base address.
	 */
	uint32_t reg(uint32_t offset) {
		return *(volatile uint32_t *)(_base + offset);
	}

	/**
	 * Write a register
	 *
	 * @param offset	Register offset in bytes from the base address.
	 * @param value	Value to write.
	 */
	void		reg(uint32_t offset, uint32_t value) {
		*(volatile uint32_t *)(_base + offset) = value;
	}

	/**
	 * Modify a register
	 *
	 * Note that there is a risk of a race during the read/modify/write cycle
	 * that must be taken care of by the caller.
	 *
	 * @param offset	Register offset in bytes from the base address.
	 * @param clearbits	Bits to clear in the register
	 * @param setbits	Bits to set in the register
	 */
	void		modify(uint32_t offset, uint32_t clearbits, uint32_t setbits) {
		uint32_t val = reg(offset);
		val &= ~clearbits;
		val |= setbits;
		reg(offset, val);
	}

private:
	uint32_t	_base;
};

} // namespace device

#endif /* _DEVICE_DEVICE_H */