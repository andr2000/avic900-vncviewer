#ifndef BRIGHTNESS_H_
#define BRIGHTNESS_H_

struct hw_module_t;
struct light_device_t;

class Brightness
{
public:
	Brightness() = default;
	virtual ~Brightness();

	bool initialize();
	bool setBrightness(int value);

private:
	light_device_t *m_LightDevice { nullptr };
	light_device_t *getLightDevice(hw_module_t *module, char const *name);
};

#endif /* BRIGHTNESS_H_ */
