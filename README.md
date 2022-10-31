# Projekt-ESP32-Nextion
Ein C++ Programm für einen ESP32, entwickelt mit VSCode und der Arduino IDE Erweiterung.
Dies ist mein erstes bei gitHub hochgeladenes Programm. Es fehlt jegliche Beschreibung oder Dokumentation.
Ich muss mich noch in github, git und vsCode mit git einarbeiten. Auch markdown ist für mich noch ein schwarzes Loch. Im Moment habe ich einfach alles was ich bisher lokal hatte in dieses repository geladen.

## Ziel des Programms
Dieses Programm soll auf möglichst einfache Weise eine Verbindung zu einem Nextion Display herstellen und Daten aus unterschiedlichen Systemen die der ESP bedienen kann darstellen und/oder steuern. Bis zu diesem Zeitpunkt (10.2022) habe ich nur das im NSPanel(EU) von Sonoff eingebaute Nextion Display.
Für mein "Smart Home" benutze ich den ioBroker. Um darin Daten zu lesen und zu steuern kann über einen MQTT Adapter oder den REST-API Adapter auf den ioBroker zugegriffen werden.
Alles was in diesem Programm entwickelt wird, wird zwar mit einer möglicht großen allgemeinen Gültigkeit getan ist aber nur auf der mir zur Verfügung stehenden Hardware entwickelt und getestet.

## Nextion Editor
Die zugehörige HMI Datei habe ich mit dem Nextion Editor V1.63.3 (https://nextion.tech/editor_guide/) erstellt. Das Display kann mit dem Simulator des Editors ohne vorhandenes Display voll getestet werden, wenn die zweite serielle Schnittstelle des ESP über einen FTDI Adapter mit einen zweiten USB Port des PCs verbunden wird. Im Nextion Editor wird dann der Debuger gestartet und darin unten 'User MCU Input' gewählt. Dort muss dann die Com Schnittstelle gewählt, und dir Baudrate auf 921600 eingestellt werden.
