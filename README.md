# Projekt-ESP32-Nextion
Ein C++ Programm für einen ESP32, entwickelt mit VSCode und der Arduino IDE Erweiterung.
Dies ist mein erstes bei github hochgeladenes Programm. Es fehlt jegliche Beschreibung oder Dokumentation.
Ich muss mich noch in github, git und vsCode mit git einarbeiten. Im Moment habe ich einfach alles was ich bisher lokal hatte in dieses repository geladen.

## Vorgeschichte
Da ich jemand bin der gerne mal etwas kauft nur um es auszuprobieren oder auseinander zu nehmen, bin ich auf das NSPanel von Sonoff gestoßen. Ich wollte damit probieren, ob und wie es sich in meinen [iobroker](https://www.iobroker.net/) integrieren ließe. Ich bin dann ziemlich schnell zu [dieser](https://github.com/joBr99/nspanel-lovelace-ui) Seite gekommen und habe damit begonnen alles wie dort beschrieben einzurichten. Doch schon bald habe ich für mich entschieden, das dabei zu viele Vorbedingungen im iobroker zu installieren und einzurichten sind und das ganze System extrem starr ist.
Also begann ich damit mich mit dem Display und der Schnittstelle dazu zu befassen. Nach vielen Stunden suchen, lesen und basteln habe ich dann begonnen kleine Testprogramme zu schreiben und mich entschieden eine eigene Firmware für den ESP32 zu entwickeln. Dabei soll soviel wie möglich über die HMI-Datei des Displays geregelt/konfiguriert werden.
Die HMI-Datei habe ich mit dem [Nextion-Editor](https://nextion.tech/nextion-editor/) 1.63.3 entwickelt.

## Ziel des Programms
Dieses Programm soll auf möglichst einfache Weise eine Verbindung zu einem Nextion Display herstellen und Daten aus unterschiedlichen Systemen, die der ESP bedienen kann, darstellen und/oder steuern. Bis zu diesem Zeitpunkt (10.2022) habe ich nur das im NSPanel(EU) von Sonoff eingebaute Nextion Display.
Um Daten aus dem iobroker lesen und schreiben zu können, sind im iobroker der [MQTT](https://github.com/ioBroker/ioBroker.mqtt) Adapter oder der [REST-API](https://github.com/ioBroker/ioBroker.rest-api) Adapter verfügbar. Ich werde in meinem Projekt mit großer Wahrscheinlichkeit das MQTT Protokoll verwenden.
Alles was in diesem Programm entwickelt wird, wird zwar mit einer möglicht großen allgemeinen Gültigkeit getan ist aber nur auf der mir zur Verfügung stehenden Hardware entwickelt und getestet.
Ich habe leider keine Zeit für Entwicklungen die mir selber nichts bringen.

## Nextion Editor
Die zugehörige HMI Datei habe ich mit dem Nextion Editor V1.63.3 (https://nextion.tech/editor_guide/) erstellt. Das Display kann mit dem Simulator des Editors ohne vorhandenes Display voll getestet werden, wenn die zweite serielle Schnittstelle des ESP über einen FTDI Adapter mit einen zweiten USB Port des PCs verbunden wird. Im Nextion Editor wird dann der Debuger gestartet und darin unten 'User MCU Input' gewählt. Dort muss dann die Com Schnittstelle gewählt, und dir Baudrate auf 921600 eingestellt werden.
