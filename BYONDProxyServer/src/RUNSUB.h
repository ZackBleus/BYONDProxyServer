#include <cstdint>
/**
 * Implementation of RUNSUB.
 * This implementation operates on whole buffers at a time in-place.
 */

namespace RUNSUB
{
	/**
		* Decrypts data using RUNSUB.
		* The last byte is checked against the checksum but otherwise ignored.
		* If the decryption fails, RUNSUBFailedException is thrown, but the data is put in the buffer.
		*
		* @throws RUNSUBFailedException On checksum failure
		*/
	static void decrypt(unsigned char data[], int offset, int length, int key) {
		char checksum = 0;

		if (key == 0)
			return;

		for (int i = 0; i < length - 1; i++) {
			char roundKey = (char)(checksum + (key >> (checksum % 32)));
			// decryption: apply checksum after round key (to get decrypted value)
			data[i + offset] -= roundKey;
			checksum += data[i + offset];
		}
		/*
		if (checksum != data[(length - 1) + offset])
			throw new RUNSUBFailedException(data[(length - 1) + offset], checksum);
		*/
	}

	/**
		* Encrypts data using RUNSUB.
		* The last byte is overwritten with the checksum.
		*/
	static void encrypt(uint8_t data[], int offset, int length, int key) {
		uint8_t checksum = 0;
		for (int i = 0; i < length - 1; i++) {
			uint8_t roundKey = (uint8_t)(checksum + (key >> (checksum % 32)));
			// encryption: apply checksum before round key (to get decrypted value)
			checksum += data[i + offset];
			data[i + offset] += roundKey;
		}
		data[(length - 1) + offset] = checksum;
	}
}