# Dispersion Estimator extension for OCTproZ
Dispersion Estimator is a plugin for [OCTproZ](https://github.com/spectralcode/OCTproZ).<br><br>

<p align="center">
	<img src="images/dispersion_estimator_screenshot.png" width="250">
</p>

The Dispersion Estimator estimates suitable dispersion values d2 and d3 by varying these parameters and optimizing an A-scan quality metric.

You can use the [SocketStreamExtension](https://github.com/spectralcode/SocketStreamExtension) to remotely start the estimation process by sending the command **`remote_plugin_control, Dispersion Estimator, startSingleFetch`**

## License
Dispersion Estimator is licensed under GPLv3. See [LICENSE](LICENSE).
