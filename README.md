## Hinweis
Dies ist mein erstes bei github hochgeladenes Projekt. Es fehlt noch an weitere Dokumentationen, Beschreibungen und Fotos. Dies werde ich irgendwann nachholen.

## Vorgeschichte
Ich bin jemand bin der gerne mal etwas kauft nur um es auszuprobieren oder auseinander zu nehmen. Dabei bin ich auf das NSPanel von Sonoff aufmerksam geworden. Ich wollte ausprobieren, ob und wie es sich mit meinen [iobroker](https://www.iobroker.net/) integrieren lässt.

# Projekt-ESP32-Nextion
Ein Projekt mit einem ESP32 Mikrocontroller, der Daten an ein Nexion display sendet. Entwickelt mit dem VSCode Editor und der Arduino IDE VSCode Erweiterung. Das Programm ließe sich aber auch mit der  Arduino IDE kompilieren und hochladen. Ich hab bei mir die [Arduino IDE 1.8.13](https://www.arduino.cc/en/software/OldSoftwareReleases) installiert, da Versionen ab 2.0 nicht mit der Erweiterung für VSCode funktionieren.

Bei meinen Recherchen dafür, bin ich ziemlich schnell zu [nspanel-lovelace](https://github.com/joBr99/nspanel-lovelace-ui) Seite gekommen und habe damit begonnen alles wie dort beschrieben einzurichten. Doch schon sehr bald habe ich für mich entschieden, dass dabei zu viele Vorbedingungen im iobroker zu installieren und einzurichten sind und das ganze System selbst für mich der ich doch schon viel Erfahrung habe, extrem kompliziert und starr ist.

Also begann ich damit mich mit dem Display und den Protokollen zu befassen. Nach vielen Stunden suchen, lesen und basteln habe ich mich entschieden eine eigene Software für den ESP32 zu entwickeln. Dabei soll soviel wie möglich über die HMI-Datei des Displays gesteuert/konfiguriert werden, um so wenig wie nötig im Programm anzupassen.


## Ziel des Projekts
Dieses Projekts soll auf möglichst einfache Weise eine Verbindung zu einem Nextion Display herstellen und Daten aus unterschiedlichen Systemen, die der ESP bedienen kann, darstellen und/oder steuern. Im Moment besitze ich nur das im [NSPanel(EU)](https://haus-automatisierung.com/hardware/sonoff/2021/10/15/sonoff-nspanel-ersteindruck.html) von Sonoff eingebaute Nextion Display.
Zum Testen benutze ich allerdings fast nur den Simulator im debug Modus des Nextion Editors und einen [‚ESP32  Dev Module‘](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/hw-reference/esp32/get-started-devkitc.html).
Das Display aus dem NSPanel hat eine Besonderheiten, die im Simulator berücksichtigt werden muss, damit das Ergebnis auf dem physikalischen Display genauso wie im Simulator aussieht. Der Rahmen des NSPanel EU verdeckt auf der rechten Seite das Display, so dass dort ca. 28 Pixel verdeckt sind. Dies wirkt sich auf die Justierung der Berührungen des Bildschirms aus. Deshalb ist in der HMI-Datei im Skript 'Program.s' ein Befehl enthalten, der dieses Manko beseitigt.

Um Daten aus dem iobroker lesen und schreiben zu können, sind im [iobroker](https://www.iobroker.net/) der [MQTT](https://github.com/ioBroker/ioBroker.mqtt) Adapter oder der [REST-API](https://github.com/ioBroker/ioBroker.rest-api) Adapter verfügbar. Ich werde in diesem Projekt mit dem MQTT Protokoll arbeiten, da dafür mehrere Bibliotheken für den ESP32 und die Arduino IDE zur Verfügung stehen.


Alles was in diesem Programm entwickelt wird, wird zwar mit einer möglichst großen allgemeinen Gültigkeit getan, ist aber nur auf der mir zur Verfügung stehenden Hardware entwickelt und getestet. Außerdem ist mein primäres Ziel, dass es in meinem Umfeld funktioniert.
Da ich mich aber mit allem hier gut auskenne, bin ich selbstverständlich bereit auch bei jeder Art von Problemen zu helfen.

## Nextion Editor
Die HMI-Datei habe ich mit dem [Nextion-Editor](https://nextion.tech/nextion-editor/) 1.65.0 entwickelt. Das Display kann mit dem Simulator des Editors ohne physikalisch vorhandene Hardware getestet werden, wenn die zweite serielle Schnittstelle des ESP über einen FTDI Adapter mit einen zweiten USB Port des PCs verbunden wird.
Im Nextion Editor wird dazu der Debuger gestartet und darin unten 'User MCU Input' gewählt. Dort muss die Com Schnittstelle des FTDI Adapters gewählt, und die Baudrate auf 921600 (ist in der HMI-Datei so gesetzt) eingestellt werden.
Dann muss, wenn der ESP läuft, im debug Fenster des Nextion-Editors im Menü 'Operation' / 'Reboot the Simulator' gewählt werden, damit der ESP den Startvorgang des Displays erkennt.

## Voraussetzungen in der HMI-Datei
- Die Baudrate muss auf 921600 gesetzt werden, da dies so in der Anwendung beim initialisieren der entsprechenden seriellen Schnittstelle eingestellt ist.
- Die Font Ressourcen müssen die von der Anwendung gesendeten Daten (UTF-8) darstellen können. Sollten also in der HMI-Datei im UTF-8 Format erstellt werden.
-