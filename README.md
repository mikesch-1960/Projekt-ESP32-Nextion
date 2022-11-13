# Projekt-ESP32-Nextion
Ein Programm für einen ESP32, entwickelt mit VSCode und der Arduino IDE Erweiterung.
Dies ist mein erstes bei github hochgeladenes Projekt. Es fehlt noch an Dokumentationen und Beschreibungen.

## Vorgeschichte
Da ich jemand bin der gerne mal etwas kauft nur um es auszuprobieren oder auseinander zu nehmen, bin ich auf das NSPanel von Sonoff gestoßen. Ich wollte damit ausprobieren, ob und wie es in meinen [iobroker](https://www.iobroker.net/) integrieren läßt. Bei meinen Recherchen bin ich ziemlich schnell zu [dieser](https://github.com/joBr99/nspanel-lovelace-ui) Seite gekommen und habe damit begonnen alles wie dort beschrieben einzurichten. Doch schon bald habe ich für mich entschieden, das dabei zu viele Vorbedingungen im iobroker zu installieren und einzurichten sind und das ganze System extrem kompliziert uns starr ist.
Also begann ich damit mich mit dem Display und der Schnittstelle dazu zu befassen. Nach vielen Stunden suchen, lesen und basteln habe ich dann begonnen kleine Testprogramme zu schreiben und mich dann entschieden eine eigene Firmware für den ESP32 zu entwickeln. Dabei soll soviel wie möglich über die HMI-Datei des Displays gesteuert/konfiguriert werden.


## Ziel des Programms
Dieses Programm soll auf möglichst einfache Weise eine Verbindung zu einem Nextion Display herstellen und Daten aus unterschiedlichen Systemen, die der ESP bedienen kann, darstellen und/oder steuern. Im Moment (10.2022) besitze ich nur das im [NSPanel(EU)](https://haus-automatisierung.com/hardware/sonoff/2021/10/15/sonoff-nspanel-ersteindruck.html) von Sonoff eingebaute Nextion Display.
Zum Testen benutze ich allerding nur den Simulator im debug Modus des Nextion Editors.
Das Display aus dem NSPanel hat ein eine Besonderheiten, die im Simulator berücksichtigt werden muss, damit das Ergebnis auf dem physikalischen Display genauso wie im Simulator aussieht. Der Rahmen des NSPanel EU verdeckt auf der einen Seite das Display, so dass dort ca. 28 Pixel verdeckt sind. Dies wirkt sich auf die Justierung der Berührungen des Bildschirms aus. Deshalb ist in der HMI-Datei im Skript 'Program.s' ein Befehl enthalten, der dieses Manko beseitigt.
Um Daten aus dem iobroker lesen und schreiben zu können, sind im iobroker der [MQTT](https://github.com/ioBroker/ioBroker.mqtt) Adapter oder der [REST-API](https://github.com/ioBroker/ioBroker.rest-api) Adapter verfügbar. Ich werde in meinem Projekt mit großer Wahrscheinlichkeit das MQTT Protokoll verwenden.


Alles was in diesem Programm entwickelt wird, wird zwar mit einer möglicht großen allgemeinen Gültigkeit getan ist aber nur auf der mir zur Verfügung stehenden Hardware entwickelt und getestet.

## Nextion Editor
Die HMI-Datei habe ich mit dem [Nextion-Editor](https://nextion.tech/nextion-editor/) 1.63.3 entwickelt. Das Display kann mit dem Simulator des Editors ohne vorhandene Hardware getestet werden, wenn die zweite serielle Schnittstelle des ESP über einen FTDI Adapter mit einen zweiten USB Port des PCs verbunden wird.
Im Nextion Editor wird dazu der Debuger gestartet und darin unten 'User MCU Input' gewählt. Dort muss die Com Schnittstelle des FTDI Adapters gewählt, und die Baudrate auf 921600 (ist in der HMI-Datei so gesetzt) eingestellt werden.
Dann sollte, wenn der ESP laüft, im debug Fenster des Nextion-Editors im Menü 'Operation' / 'Reboot the Simulator' gewählt werden.