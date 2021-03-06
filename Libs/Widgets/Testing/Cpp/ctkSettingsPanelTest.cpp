/*=========================================================================

  Library:   CTK

  Copyright (c) Kitware Inc.

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0.txt

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

=========================================================================*/

// Qt includes
#include <QApplication>
#include <QSettings>
#include <QSignalSpy>
#include <QSpinBox>

// CTK includes
#include "ctkSettingsPanel.h"
#include "ctkSettingsPanelTest2Helper.h"
#include "ctkTest.h"

// STD includes
#include <cstdlib>
#include <iostream>


// ----------------------------------------------------------------------------
class ctkSettingsPanelTester: public QObject
{
  Q_OBJECT
private slots:

  void testChangeProperty();
  void testChangeProperty_data();

  void testEmptyQStringList();
};

//-----------------------------------------------------------------------------
void ctkSettingsPanelTester::testChangeProperty()
{
  QSettings settings(QSettings::IniFormat, QSettings::UserScope, "Common ToolKit", "CTK");
  QSpinBox spinBox;
  ctkSettingsPanel panel;
  panel.setSettings(&settings);
  spinBox.setValue(1);

  QFETCH(QString, label);
  QFETCH(ctkSettingsPanel::SettingOptions, options);
  panel.registerProperty("property", &spinBox,
                         "value", SIGNAL(valueChanged(int)),
                         label, options);

  QCOMPARE(spinBox.value(), 1);
  QCOMPARE(panel.settingLabel("property"), label);
  QCOMPARE(panel.settingOptions("property"), options);

  qRegisterMetaType<QVariant>("QVariant");
  QSignalSpy spy(&panel, SIGNAL(settingChanged(QString,QVariant)));
  QFETCH(int, value);
  QFETCH(bool, setOnObject);
  if (setOnObject)
    {
    spinBox.setValue(value);
    }
  else
    {
    panel.setSetting("property", QVariant(value));
    }

  QFETCH(int, expectedSettingChangedCount);
  QCOMPARE(spy.count(), expectedSettingChangedCount);
  QFETCH(QStringList, expectedChangedSettings);
  QCOMPARE(panel.changedSettings(), expectedChangedSettings);

  // make sure it didn't change
  QCOMPARE(panel.settingLabel("property"), label);
  QCOMPARE(panel.settingOptions("property"), options);

  panel.resetSettings();
}

//-----------------------------------------------------------------------------
void ctkSettingsPanelTester::testChangeProperty_data()
{
  QTest::addColumn<QString>("label");
  QTest::addColumn<ctkSettingsPanel::SettingOptions>("options");
  QTest::addColumn<int>("value");
  QTest::addColumn<bool>("setOnObject");
  QTest::addColumn<int>("expectedSettingChangedCount");
  QTest::addColumn<QStringList>("expectedChangedSettings");

  QTest::newRow("null none changed obj") << QString() << ctkSettingsPanel::SettingOptions(ctkSettingsPanel::OptionNone) << 2 << true << 1 << QStringList("property");
  QTest::newRow("null none changed panel") << QString() << ctkSettingsPanel::SettingOptions(ctkSettingsPanel::OptionNone) << 2 << false << 1 << QStringList("property");
  QTest::newRow("null none unchanged obj") << QString() << ctkSettingsPanel::SettingOptions(ctkSettingsPanel::OptionNone) << 1 << true << 0 << QStringList();
  QTest::newRow("null none unchanged panel") << QString() << ctkSettingsPanel::SettingOptions(ctkSettingsPanel::OptionNone) << 1 << false << 0 << QStringList();
  QTest::newRow("null RequireRestart changed obj") << QString() << ctkSettingsPanel::SettingOptions(ctkSettingsPanel::OptionRequireRestart) << 2 << true << 1 << QStringList("property");
  QTest::newRow("null RequireRestart changed panel") << QString() << ctkSettingsPanel::SettingOptions(ctkSettingsPanel::OptionRequireRestart) << 2 << false << 1 << QStringList("property");
  QTest::newRow("null RequireRestart unchanged obj") << QString() << ctkSettingsPanel::SettingOptions(ctkSettingsPanel::OptionRequireRestart) << 1 << true << 0 << QStringList();
  QTest::newRow("null RequireRestart unchanged panel") << QString() << ctkSettingsPanel::SettingOptions(ctkSettingsPanel::OptionRequireRestart) << 1 << false << 0 << QStringList();
  QTest::newRow("empty none changed obj") << QString("") << ctkSettingsPanel::SettingOptions(ctkSettingsPanel::OptionNone) << 2 << true << 1 << QStringList("property");
  QTest::newRow("empty none changed panel") << QString("") << ctkSettingsPanel::SettingOptions(ctkSettingsPanel::OptionNone) << 2 << false << 1 << QStringList("property");
  QTest::newRow("empty none unchanged obj") << QString("") << ctkSettingsPanel::SettingOptions(ctkSettingsPanel::OptionNone) << 1 << true << 0 << QStringList();
  QTest::newRow("empty none unchanged panel") << QString("") << ctkSettingsPanel::SettingOptions(ctkSettingsPanel::OptionNone) << 1 << false << 0 << QStringList();
  QTest::newRow("empty RequireRestart changed obj") << QString("") << ctkSettingsPanel::SettingOptions(ctkSettingsPanel::OptionRequireRestart) << 2 << true << 1 << QStringList("property");
  QTest::newRow("empty RequireRestart changed panel") << QString("") << ctkSettingsPanel::SettingOptions(ctkSettingsPanel::OptionRequireRestart) << 2 << false << 1 << QStringList("property");
  QTest::newRow("empty RequireRestart unchanged obj") << QString("") << ctkSettingsPanel::SettingOptions(ctkSettingsPanel::OptionRequireRestart) << 1 << true << 0 << QStringList();
  QTest::newRow("empty RequireRestart unchanged panel") << QString("") << ctkSettingsPanel::SettingOptions(ctkSettingsPanel::OptionRequireRestart) << 1 << false << 0 << QStringList();
  QTest::newRow("label none changed obj") << QString("label") << ctkSettingsPanel::SettingOptions(ctkSettingsPanel::OptionNone) << 2 << true << 1 << QStringList("property");
  QTest::newRow("label none changed panel") << QString("label") << ctkSettingsPanel::SettingOptions(ctkSettingsPanel::OptionNone) << 2 << false << 1 << QStringList("property");
  QTest::newRow("label none unchanged obj") << QString("label") << ctkSettingsPanel::SettingOptions(ctkSettingsPanel::OptionNone) << 1 << true << 0 << QStringList();
  QTest::newRow("label none unchanged panel") << QString("label") << ctkSettingsPanel::SettingOptions(ctkSettingsPanel::OptionNone) << 1 << false << 0 << QStringList();
  QTest::newRow("label RequireRestart changed obj") << QString("label") << ctkSettingsPanel::SettingOptions(ctkSettingsPanel::OptionRequireRestart) << 2 << true << 1 << QStringList("property");
  QTest::newRow("label RequireRestart changed panel") << QString("label") << ctkSettingsPanel::SettingOptions(ctkSettingsPanel::OptionRequireRestart) << 2 << false << 1 << QStringList("property");
  QTest::newRow("label RequireRestart unchanged obj") << QString("label") << ctkSettingsPanel::SettingOptions(ctkSettingsPanel::OptionRequireRestart) << 1 << true << 0 << QStringList();
  QTest::newRow("label RequireRestart unchanged panel") << QString("label") << ctkSettingsPanel::SettingOptions(ctkSettingsPanel::OptionRequireRestart) << 1 << false << 0 << QStringList();
}

//-----------------------------------------------------------------------------
void ctkSettingsPanelTester::testEmptyQStringList()
{
  {
    // When QSettings goes out of scope, we are the settings file is up-to-date
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "Common ToolKit", "CTK");
    settings.clear();
    settings.setValue("list", QVariant(QStringList()));
  }

  // Regression: Reading empty QStringList property from settings should be handled properly

  ctkSettingsPanel settingsPanel;
  ctkSettingsPanelTest2Helper * list = new ctkSettingsPanelTest2Helper(&settingsPanel);
  settingsPanel.registerProperty("list", list, "list", SIGNAL(listChanged()));
  QSettings settings2(QSettings::IniFormat, QSettings::UserScope, "Common ToolKit", "CTK");
  settingsPanel.setSettings(&settings2);
}

// ----------------------------------------------------------------------------
CTK_TEST_MAIN(ctkSettingsPanelTest)
#include "moc_ctkSettingsPanelTest.cpp"
