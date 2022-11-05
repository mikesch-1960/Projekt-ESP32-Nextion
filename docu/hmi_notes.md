## todos:
  - Im swipe touch, bei der Abfrage ob der touch an einer erlaubten Position startet, nicht nach einem Hintergrundbild mit einer bestimmten Position, sondern einem bestimten Objektnamen fragen. Geht das?
  - Bei swipe: die Ergebnis-Berechnung in cmdCalc button verschieben. Dann kann der Timer oder auch ein langsamerer die Berechnung ausführen und so während des wischens Informationen anzeigen/anpassen

## Ideen:
  - Testen ob mit Seiten die "no background" haben eine Seite mit tabs gebaut werden kann.
  - Das setzen des Standby Modus übernimmt der ESP. Damit ist weniger im HMI zu tun.
  - Das setzen des longtouch Modus übernimmt der ESP. Damit ist weniger im HMI zu tun.


# Notizen

## Spezielle Komponenten zum Anzeigen von Daten die der ESP liefert
### Allgemeines:
  Daten die durch den ESP aktualisiert werden sollen, müssen Komponenten verwenden deren Name mit einem Unterstrich beginnen. Danach muss der Name der zu aktualisierenden Daten folgen. Es können auch globale integer Variablen im 'Program.s' Skript aktualisiert werden.
  Nur mit dieser Maßnahme geschieht jedoch erstmal nichts. Beim 'preinitialize' der Seite müssen die zu aktualisierenden Komponenten dem ESP bekannt gegeben werden. Dazu muss dem ESP der Befehl '<PIcompNam1.ext&param;...;FF FF FF' gesendet werden. die mit diesem Befehl gesendeten Informationen sehen je nach zu aktualisierenden Komponenten etwas anders aus. Das Prinzip ist aber immer dasselbe. Es wird eine Liste mit den Komponentennamen (inkl. Erweiterung), plus dazugehörigem Parameter gesendet.

### Zeitkomponenten (_time):
  Um im Display Zeiten anzuzeigen müssen die Komponentennamen mit '_time' beginnen. Der Rest des Namens kann Vom Benutzer frei gewählt werden.
  Damit der ESP die zu aktualisierenden Komponenten kennt, müssen diese im oben beschriebenen Befehl der an den ESP gesendet werden muss bekannt gemacht werden. Dazu ein Beispiel:
  Eine Seite in der die Uhrzeit und das Datum angezeigt werden soll. Also werden die entsprechenden Text Komponenten _time und _timeDate im Formular erstellt. Der Befehl für den ESP könnte dann so aussehen:
```javascript
    prints "<PI",0  // Kennung des Befehls 'page init'
    prints "_time.txt&%H%M;_timeDate.txt&%d.%m.%Y;",0  // Liste der Komponenten und das anzuzeigende Format
    printh FF FF FF // Ende des Befehls
```
  Die Komponenten würden dem ESP beim Öffnen der Seite bekannt gegeben und dieser würde diese Komponenten dann regelmäßig aktualisieren, indem der ESP das Kommando zum setzen der Inhalte der Komponente an die Seite sendet.
  Die Daten werden nur dann gesendet, wenn eine Aktualisierung erforderlich ist. Bei Zeitdaten ist dies normalerweise wenn die Minute wechselt. Falls in einem Zeitwert ein Formatbezeichner für eine Sekunde enthalten ist wird auch beim Wechsel der Sekunde aktualisiert.
  Hinter dem Komponentennamen in der Liste wird also das Anzeigeformat angegeben. Die möglichen Formatbezeichner sind [hier](https://help.gnome.org/users/gthumb/unstable/gthumb-date-formats.html.de) alle beschrieben.
  Es können aber auch noch bis zu vier vordefinierte Formatbezeichner in der Konfiguration der HMI Datei definiert werden. Diese Formatmakros können anstatt eines Formatausdrucks angegeben werden, indem statt eines Zeitformats '#0' - '#3' angegeben wird. Dies ist insbesondere nützlich, wenn auf vielen Seiten eine Zeitkomponente angezeigt werden soll und alle das Selbe Format bekommen sollen. Durch ändern des Formatmakros in der Konfiguration würde dies dann auf allen Seiten umgestellt sein.
  Bei einer Komponente mit dem Namen '_time.txt' muss kein Format angegeben sein. Diese würde dann standardmäßig '#0' verwenden.

  ### Wifikomponenten (_wifi):
  Ähnlich wie bei Zeitkomponenten, nur wird hier als Parameter nicht ein Zeitformat, sondern ein sogenanntes Datenformat angegeben. Dieses bestimmt welche Daten des Wifi dargestellt werden sollen. Mögliche Angaben sind:
  %N      SSID als Text
  %M todo MAC
  &R      RSSI in dBa (0..-100)
  &Q      Qualität in % (0..100; ab -50=100%)
  &Q<a,b> Qualität im angegebenen Bereich. Kann für die Darstellung von Icons verwendet werden. Z.B. '%Q<4,8>' würde Werte zwischen 4 und 8 zurückgeben.
  %I      Ip im Textformat 'aaa.bbb.ccc.ddd'
  %G      Gateway im Textformat 'aaa.bbb.ccc.ddd'
  %S      Subnetmask im Textformat 'aaa.bbb.ccc.ddd'
  %I,G,S[n]   Bei Ip Werten kann mit dieser Angabe der n-ter Teil der Ip als Zahl angezeigt werden

  Bei den Komponenten kann sogar eine andere Eigenschaft als .txt oder .val verwendet werden. So kann mit Hilfe der .pic Eigenschaft einer Picture Komponente ein der Verbindungsqualität entspreches Icon angezeigt werden. (siehe Beispiel in der pgStandby Seite der HMI-Datei)

## Standby:
Bei einem Standbymodus wird eine extra Seite angezeigt und das Backlight des Displays gedimmt. Beim Berühren der Stanbyseite wird das Backlight wieder hell geschaltet und beim Loslassen die zuvor angezeigte Seite angezeigt.
Mit einem einfachen touch oder einem wischen nach links wird die vorherige Seite wieder angezeigt.
Mit einem wischen nach oben die Homepage, nach rechts die Quickmenüseite und nach unten die Configseite.
Hinweis: Sind auf der Seite von der aus auf die Stanbyseite gewechselt wird Werte von Komponenten verändert worden, sind diese nach einem Standby wieder auf den Standardwert zurückgesetzt.
Um das zu verhindern, müssen in den Seitenereignissen "Preinitialize" und "page exit" die Werte in globalen Variablen zwischengespeichert, oder die ensprechenden Komponenten auf vscope global umgestellt werden (kostet Speicher). Komponenten die Daten aus dem ESP erhalten werden automatisch beim Laden der Seite wieder mit den aktuellen Daten geladen.

Ansonsten muss nur eine Timer Komponente mit dem Namen stbyTimer zur Seite hinzugefügt werden der im Event folgenden Code hat:

```javascript
if(stbyTimer.tim==50)  // 50 is only for initializing the configured value
{
  // set the period to the configured value
  stbyTimer.tim=cfgStbyDelay*1000 // cfgStbyDelay is in seconds
  // restart timer with new period
  stbyTimer.en=1
}else
{
  // reset the timer, so that on next page open the timer restarts with full period
  stbyTimer.en=1
  // Save the page ID on which standby mode was triggered
  stbyFromPage=dp
  // goto standby page
  page pgStandby
}
```
Den Rest erledigt die Standbyseite.


## Komunikation mit der MCU:
  'page init' Ereignis:
    Enthält die Seite Komponenten, deren Inhalt durch die MCU gefüllt werden sollen, müssen diese Komponenten der MCU bekannt gemacht werden.
    Dies geschieht im 'Preinitialize Event' der Seite, indem der MCU die folgende Meldung gesendet wird:
    ```javescript
      prints "<PI",0        // Page Init - definiert die Art der Meldung
      prints "...",0        // Eine Liste der Komponenten und dazu gehörige Parameter im Format compname.ext&param;
      printh FF FF FF       // Ende der Meldung
    ```
    Namen zu aktualisierender Komponenten beginnen mit `_`.

    Für Komponenten mit der aktuellen Uhrzeit, müssen die Namen mit `_time` beginnen. Um zu bestimmen wie die Daten in der Komponente angezeigt werden, kann das Format im Parameter angegeben werden.
    [Hier](https://help.gnome.org/users/gthumb/unstable/gthumb-date-formats.html.de) sind alle Formate beschrieben.
    Maximal können 50 Komponenten in diese Liste eingetragen werden.
    Für die `_time.txt` Komponente wird als Vorgabeformat das erste Formatmakro verwendet. Bei Komponenten die andere Angaben zeigen sollen müssen die Namen der Komponenten ebenfalls mit `_time` beginnen und der Parameter für das Format ist zwingend anzugeben. Ach die Angabe eines Formatmakros (#0-#3) ist erlaubt.
    Beispiel für nicht standard Komponenten wie sie im init string der Seite enthalten sein können:
      `_timeH_asNum.val&%H;` // Stunde als Zahl
      `_timeH_AndSec.txt&%H:%M.%S;` // Zeit mit Sekunden als Text
    Bei Komponenten mit einer anderen Erweiterung als `.txt`, oder ohne Erweiterung (globale integer Variable aus Program.s) muss das angegebene Format eine Zahl ergeben!
    Komponenten die mit `_time` beginnen werden beim Betreten der Seite und danach bei jedem Wechsel der Minute, oder falls die Komponente ein Format mit einem Sekundenanteil hat, bei jedem Wechsel der Sekunde aktualisiert.
