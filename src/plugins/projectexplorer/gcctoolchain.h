/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include "projectexplorer_export.h"

#include "projectexplorerconstants.h"
#include "toolchain.h"
#include "abi.h"
#include "headerpath.h"

#include <utils/fileutils.h>

#include <functional>
#include <memory>

namespace ProjectExplorer {

namespace Internal {
class ClangToolChainFactory;
class ClangToolChainConfigWidget;
class GccToolChainConfigWidget;
class GccToolChainFactory;
class MingwToolChainFactory;
class LinuxIccToolChainFactory;
}

// --------------------------------------------------------------------------
// GccToolChain
// --------------------------------------------------------------------------

inline const QStringList languageOption(Core::Id languageId)
{
    if (languageId == Constants::C_LANGUAGE_ID)
        return {"-x", "c"};
    return {"-x", "c++"};
}

inline const QStringList gccPredefinedMacrosOptions(Core::Id languageId)
{
    return languageOption(languageId) + QStringList({"-E", "-dM"});
}

class PROJECTEXPLORER_EXPORT GccToolChain : public ToolChain
{
public:
    GccToolChain(Core::Id typeId);
    QString typeDisplayName() const override;
    Abi targetAbi() const override;
    QString originalTargetTriple() const override;
    QString version() const;
    QList<Abi> supportedAbis() const override;
    void setTargetAbi(const Abi &);

    bool isValid() const override;

    Utils::LanguageExtensions languageExtensions(const QStringList &cxxflags) const override;
    WarningFlags warningFlags(const QStringList &cflags) const override;

    MacroInspectionRunner createMacroInspectionRunner() const override;
    Macros predefinedMacros(const QStringList &cxxflags) const override;

    BuiltInHeaderPathsRunner createBuiltInHeaderPathsRunner() const override;
    HeaderPaths builtInHeaderPaths(const QStringList &flags,
                                   const Utils::FileName &sysRootPath) const override;

    void addToEnvironment(Utils::Environment &env) const override;
    Utils::FileName makeCommand(const Utils::Environment &environment) const override;
    Utils::FileNameList suggestedMkspecList() const override;
    IOutputParser *outputParser() const override;

    QVariantMap toMap() const override;
    bool fromMap(const QVariantMap &data) override;

    std::unique_ptr<ToolChainConfigWidget> createConfigurationWidget() override;

    bool operator ==(const ToolChain &) const override;

    void resetToolChain(const Utils::FileName &);
    Utils::FileName compilerCommand() const override;
    void setPlatformCodeGenFlags(const QStringList &);
    QStringList extraCodeModelFlags() const override;
    QStringList platformCodeGenFlags() const;
    void setPlatformLinkerFlags(const QStringList &);
    QStringList platformLinkerFlags() const;

    static void addCommandPathToEnvironment(const Utils::FileName &command, Utils::Environment &env);

    class DetectedAbisResult {
    public:
        DetectedAbisResult() = default;
        DetectedAbisResult(const QList<Abi> &supportedAbis,
                           const QString &originalTargetTriple = QString()) :
            supportedAbis(supportedAbis),
            originalTargetTriple(originalTargetTriple)
        { }

        QList<Abi> supportedAbis;
        QString originalTargetTriple;
    };

protected:
    using CacheItem = QPair<QStringList, Macros>;
    using GccCache = QVector<CacheItem>;

    void setCompilerCommand(const Utils::FileName &path);
    void setSupportedAbis(const QList<Abi> &m_abis);
    void setOriginalTargetTriple(const QString &targetTriple);
    void setMacroCache(const QStringList &allCxxflags, const Macros &macroCache) const;
    Macros macroCache(const QStringList &allCxxflags) const;

    virtual QString defaultDisplayName() const;
    virtual Utils::LanguageExtensions defaultLanguageExtensions() const;

    virtual DetectedAbisResult detectSupportedAbis() const;
    virtual QString detectVersion() const;

    // Reinterpret options for compiler drivers inheriting from GccToolChain (e.g qcc) to apply -Wp option
    // that passes the initial options directly down to the gcc compiler
    using OptionsReinterpreter = std::function<QStringList(const QStringList &options)>;
    void setOptionsReinterpreter(const OptionsReinterpreter &optionsReinterpreter);

    using ExtraHeaderPathsFunction = std::function<void(HeaderPaths &)>;
    void initExtraHeaderPathsFunction(ExtraHeaderPathsFunction &&extraHeaderPathsFunction) const;

    static HeaderPaths builtInHeaderPaths(const Utils::Environment &env,
                                          const Utils::FileName &compilerCommand,
                                          const QStringList &platformCodeGenFlags,
                                          OptionsReinterpreter reinterpretOptions,
                                          HeaderPathsCache headerCache,
                                          Core::Id languageId,
                                          ExtraHeaderPathsFunction extraHeaderPathsFunction,
                                          const QStringList &flags,
                                          const QString &sysRoot,
                                          const QString &originalTargetTriple);

    static HeaderPaths gccHeaderPaths(const Utils::FileName &gcc, const QStringList &args,
                                      const QStringList &env);

    class WarningFlagAdder
    {
    public:
        WarningFlagAdder(const QString &flag, WarningFlags &flags);
        void operator ()(const char name[], WarningFlags flagsSet);

        bool triggered() const;
    private:
        QByteArray m_flagUtf8;
        WarningFlags &m_flags;
        bool m_doesEnable = false;
        bool m_triggered = false;
    };

private:
    explicit GccToolChain();

    void updateSupportedAbis() const;
    static QStringList gccPrepareArguments(const QStringList &flags,
                                           const QString &sysRoot,
                                           const QStringList &platformCodeGenFlags,
                                           Core::Id languageId,
                                           OptionsReinterpreter reinterpretOptions);

protected:
    Utils::FileName m_compilerCommand;
    QStringList m_platformCodeGenFlags;
    QStringList m_platformLinkerFlags;

    OptionsReinterpreter m_optionsReinterpreter = [](const QStringList &v) { return v; };
    mutable ExtraHeaderPathsFunction m_extraHeaderPathsFunction = [](HeaderPaths &) {};

private:
    Abi m_targetAbi;
    mutable QList<Abi> m_supportedAbis;
    mutable QString m_originalTargetTriple;
    mutable HeaderPaths m_headerPaths;
    mutable QString m_version;

    friend class Internal::GccToolChainConfigWidget;
    friend class Internal::GccToolChainFactory;
    friend class ToolChainFactory;
};

// --------------------------------------------------------------------------
// ClangToolChain
// --------------------------------------------------------------------------

class PROJECTEXPLORER_EXPORT ClangToolChain : public GccToolChain
{
public:
    ClangToolChain();
    explicit ClangToolChain(Core::Id typeId);
    ~ClangToolChain() override;

    QString typeDisplayName() const override;
    Utils::FileName makeCommand(const Utils::Environment &environment) const override;

    Utils::LanguageExtensions languageExtensions(const QStringList &cxxflags) const override;
    WarningFlags warningFlags(const QStringList &cflags) const override;

    IOutputParser *outputParser() const override;

    Utils::FileNameList suggestedMkspecList() const override;
    void addToEnvironment(Utils::Environment &env) const override;

    QString originalTargetTriple() const override;
    QString sysRoot() const override;

    BuiltInHeaderPathsRunner createBuiltInHeaderPathsRunner() const override;

    std::unique_ptr<ToolChainConfigWidget> createConfigurationWidget() override;

    QVariantMap toMap() const override;
    bool fromMap(const QVariantMap &data) override;

protected:
    Utils::LanguageExtensions defaultLanguageExtensions() const override;
    void syncAutodetectedWithParentToolchains();

private:
    QByteArray m_parentToolChainId;
    QMetaObject::Connection m_mingwToolchainAddedConnection;
    QMetaObject::Connection m_thisToolchainRemovedConnection;

    friend class Internal::ClangToolChainFactory;
    friend class Internal::ClangToolChainConfigWidget;
    friend class ToolChainFactory;
};

// --------------------------------------------------------------------------
// MingwToolChain
// --------------------------------------------------------------------------

class PROJECTEXPLORER_EXPORT MingwToolChain : public GccToolChain
{
public:
    QString typeDisplayName() const override;
    Utils::FileName makeCommand(const Utils::Environment &environment) const override;

    Utils::FileNameList suggestedMkspecList() const override;

private:
    MingwToolChain();

    friend class Internal::MingwToolChainFactory;
    friend class ToolChainFactory;
};

// --------------------------------------------------------------------------
// LinuxIccToolChain
// --------------------------------------------------------------------------

class PROJECTEXPLORER_EXPORT LinuxIccToolChain : public GccToolChain
{
public:
    QString typeDisplayName() const override;

    Utils::LanguageExtensions languageExtensions(const QStringList &cxxflags) const override;
    IOutputParser *outputParser() const override;

    Utils::FileNameList suggestedMkspecList() const override;

private:
    LinuxIccToolChain();

    friend class Internal::LinuxIccToolChainFactory;
    friend class ToolChainFactory;
};

} // namespace ProjectExplorer
