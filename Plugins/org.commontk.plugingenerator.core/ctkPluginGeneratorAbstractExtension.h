/*=============================================================================

  Library: CTK

  Copyright (c) 2010 German Cancer Research Center,
    Division of Medical and Biological Informatics

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

=============================================================================*/


#ifndef CTKPLUGINGENERATORABSTRACTEXTENSION_H
#define CTKPLUGINGENERATORABSTRACTEXTENSION_H

#include <QObject>
#include <QHash>

class ctkPluginGeneratorAbstractExtensionPrivate;

class ctkPluginGeneratorAbstractExtension : public QObject
{
  Q_OBJECT

public:
    ctkPluginGeneratorAbstractExtension();
    virtual ~ctkPluginGeneratorAbstractExtension();

    virtual void getCommandLineArgs() const = 0;

    void setParameter(const QHash<QString, QVariant>& params);
    QHash<QString, QVariant> getParameter() const;

    bool isValid() const;

    QString getErrorMessage() const;

    void generate();

signals:

    void errorMessageChanged(const QString&);

protected:

    void setErrorMessage(const QString& errMsg);

    virtual void verifyParameter(const QHash<QString, QVariant>& params) = 0;

private:

    Q_DECLARE_PRIVATE(ctkPluginGeneratorAbstractExtension)

    const QScopedPointer<ctkPluginGeneratorAbstractExtensionPrivate> d_ptr;

};

#endif // CTKPLUGINGENERATORABSTRACTEXTENSION_H