import QtQuick 2.0
import QtQuick.Layouts 1.0
import QtQuick.Controls 1.1
import QtQuick.Controls.Styles 1.3
import QtQuick.Dialogs 1.1
import Qt.labs.settings 1.0

DefaultImgButton
{
	property var getContent;
	iconSource: "qrc:/qml/img/copyiconactive.png"
	tooltip: qsTr("Copy to clipboard")
	onClicked:
	{
		clipboard.text = getContent()
	}
}

