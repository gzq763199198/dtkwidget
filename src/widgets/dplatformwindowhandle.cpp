/*
 * Copyright (C) 2017 ~ 2017 Deepin Technology Co., Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "dplatformwindowhandle.h"
#include "dapplication.h"
#include "util/dwindowmanagerhelper.h"

#include <QWindow>
#include <QWidget>
#include <QGuiApplication>
#include <QDebug>

DWIDGET_BEGIN_NAMESPACE

#define DEFINE_CONST_CHAR(Name) const char _##Name[] = "_d_" #Name

DEFINE_CONST_CHAR(useDxcb);
DEFINE_CONST_CHAR(netWmStates);
DEFINE_CONST_CHAR(windowRadius);
DEFINE_CONST_CHAR(borderWidth);
DEFINE_CONST_CHAR(borderColor);
DEFINE_CONST_CHAR(shadowRadius);
DEFINE_CONST_CHAR(shadowOffset);
DEFINE_CONST_CHAR(shadowColor);
DEFINE_CONST_CHAR(clipPath);
DEFINE_CONST_CHAR(frameMask);
DEFINE_CONST_CHAR(frameMargins);
DEFINE_CONST_CHAR(translucentBackground);
DEFINE_CONST_CHAR(enableSystemResize);
DEFINE_CONST_CHAR(enableSystemMove);
DEFINE_CONST_CHAR(enableBlurWindow);
DEFINE_CONST_CHAR(windowBlurAreas);
DEFINE_CONST_CHAR(windowBlurPaths);
DEFINE_CONST_CHAR(autoInputMaskByClipPath);

// functions
DEFINE_CONST_CHAR(setWmBlurWindowBackgroundArea);
DEFINE_CONST_CHAR(setWmBlurWindowBackgroundPathList);
DEFINE_CONST_CHAR(setWmBlurWindowBackgroundMaskImage);

DPlatformWindowHandle::DPlatformWindowHandle(QWindow *window, QObject *parent)
    : QObject(parent)
    , m_window(window)
{
    enableDXcbForWindow(window);

    window->installEventFilter(this);
}

DPlatformWindowHandle::DPlatformWindowHandle(QWidget *widget, QObject *parent)
    : QObject(parent)
    , m_window(Q_NULLPTR)
{
    enableDXcbForWindow(widget);

    m_window = widget->window()->windowHandle();
    m_window->installEventFilter(this);
}

void DPlatformWindowHandle::enableDXcbForWindow(QWidget *widget)
{
    Q_ASSERT_X(DApplication::isDXcbPlatform(), "DPlatformWindowHandler:",
               "Must call DPlatformWindowHandler::loadDXcbPlugin before");

    QWidget *window = widget->window();

    if (window->windowHandle()) {
        Q_ASSERT_X(window->windowHandle()->property(_useDxcb).toBool(), "DPlatformWindowHandler:",
                   "Must be called before window handle has been created. See also QWidget::windowHandle()");
    } else {
        Q_ASSERT_X(!window->windowHandle(), "DPlatformWindowHandler:",
                   "Must be called before window handle has been created. See also QWidget::windowHandle()");

        /// TODO: Avoid call parentWidget()->enforceNativeChildren().
        qApp->setAttribute(Qt::AA_DontCreateNativeWidgetSiblings, true);
        window->setAttribute(Qt::WA_NativeWindow);

        Q_ASSERT_X(window->windowHandle(), "DPlatformWindowHandler:",
                   "widget window handle is NULL.");

        window->windowHandle()->setProperty(_useDxcb, true);
    }
}

void DPlatformWindowHandle::enableDXcbForWindow(QWindow *window)
{
    Q_ASSERT_X(DApplication::isDXcbPlatform(), "DPlatformWindowHandler:",
               "Must call DPlatformWindowHandler::loadDXcbPlugin before");

    if (window->handle()) {
        Q_ASSERT_X(window->property(_useDxcb).toBool(), "DPlatformWindowHandler:",
                   "Must be called before window handle has been created. See also QWindow::handle()");
    } else {
        Q_ASSERT_X(!window->handle(), "DPlatformWindowHandler:",
                   "Must be called before window handle has been created. See also QWindow::handle()");

        window->setProperty(_useDxcb, true);
    }
}

bool DPlatformWindowHandle::isEnabledDXcb(const QWidget *widget)
{
    return widget->windowHandle() && widget->windowHandle()->property(_useDxcb).toBool();
}

bool DPlatformWindowHandle::isEnabledDXcb(const QWindow *window)
{
    return window->property(_useDxcb).toBool();
}

bool DPlatformWindowHandle::setWindowBlurAreaByWM(QWidget *widget, const QVector<DPlatformWindowHandle::WMBlurArea> &area)
{
    Q_ASSERT(widget);

    return widget->windowHandle() && setWindowBlurAreaByWM(widget->windowHandle(), area);
}

bool DPlatformWindowHandle::setWindowBlurAreaByWM(QWidget *widget, const QList<QPainterPath> &paths)
{
    Q_ASSERT(widget);

    return widget->windowHandle() && setWindowBlurAreaByWM(widget->windowHandle(), paths);
}

bool DPlatformWindowHandle::setWindowBlurAreaByWM(QWindow *window, const QVector<DPlatformWindowHandle::WMBlurArea> &area)
{
    if (!window) {
        return false;
    }

    if (isEnabledDXcb(window)) {
        window->setProperty(_windowBlurAreas, QVariant::fromValue(*(reinterpret_cast<const QVector<quint32>*>(&area))));

        return true;
    }

    QFunctionPointer setWmBlurWindowBackgroundArea = Q_NULLPTR;

#if QT_VERSION >= QT_VERSION_CHECK(5, 4, 0)
    setWmBlurWindowBackgroundArea = qApp->platformFunction(_setWmBlurWindowBackgroundArea);
#endif

    if (!setWmBlurWindowBackgroundArea) {
        qWarning("setWindowBlurAreaByWM is not support");

        return false;
    }

    QSurfaceFormat format = window->format();

    format.setAlphaBufferSize(8);
    window->setFormat(format);

    return reinterpret_cast<bool(*)(const quint32, const QVector<WMBlurArea>&)>(setWmBlurWindowBackgroundArea)(window->winId(), area);
}

bool DPlatformWindowHandle::setWindowBlurAreaByWM(QWindow *window, const QList<QPainterPath> &paths)
{
    if (!window) {
        return false;
    }

    if (isEnabledDXcb(window)) {
        window->setProperty(_windowBlurPaths, QVariant::fromValue(paths));

        return true;
    }

    QFunctionPointer setWmBlurWindowBackgroundPathList = Q_NULLPTR;

#if QT_VERSION >= QT_VERSION_CHECK(5, 4, 0)
    setWmBlurWindowBackgroundPathList = qApp->platformFunction(_setWmBlurWindowBackgroundPathList);
#endif

    if (!setWmBlurWindowBackgroundPathList) {
        qWarning("setWindowBlurAreaByWM is not support");

        return false;
    }

    QSurfaceFormat format = window->format();

    format.setAlphaBufferSize(8);
    window->setFormat(format);

    return reinterpret_cast<bool(*)(const quint32, const QList<QPainterPath>&)>(setWmBlurWindowBackgroundPathList)(window->winId(), paths);
}

bool DPlatformWindowHandle::connectWindowManagerChangedSignal(QObject *object, std::function<void ()> slot)
{
    if (object) {
        return QObject::connect(DWindowManagerHelper::instance(), &DWindowManagerHelper::windowManagerChanged, object, slot);
    }

    return QObject::connect(DWindowManagerHelper::instance(), &DWindowManagerHelper::windowManagerChanged, slot);
}

bool DPlatformWindowHandle::connectHasBlurWindowChanged(QObject *object, std::function<void ()> slot)
{
    if (object) {
        return QObject::connect(DWindowManagerHelper::instance(), &DWindowManagerHelper::hasBlurWindowChanged, object, slot);
    }

    return QObject::connect(DWindowManagerHelper::instance(), &DWindowManagerHelper::hasBlurWindowChanged, slot);
}

bool DPlatformWindowHandle::setWindowBlurAreaByWM(const QVector<DPlatformWindowHandle::WMBlurArea> &area)
{
    return setWindowBlurAreaByWM(m_window, area);
}

bool DPlatformWindowHandle::setWindowBlurAreaByWM(const QList<QPainterPath> &paths)
{
    return setWindowBlurAreaByWM(m_window, paths);
}

int DPlatformWindowHandle::windowRadius() const
{
    return m_window->property(_windowRadius).toInt();
}

int DPlatformWindowHandle::borderWidth() const
{
    return m_window->property(_borderWidth).toInt();
}

QColor DPlatformWindowHandle::borderColor() const
{
    return qvariant_cast<QColor>(m_window->property(_borderColor));
}

int DPlatformWindowHandle::shadowRadius() const
{
    return m_window->property(_shadowRadius).toInt();
}

QPoint DPlatformWindowHandle::shadowOffset() const
{
    return m_window->property(_shadowOffset).toPoint();
}

QColor DPlatformWindowHandle::shadowColor() const
{
    return qvariant_cast<QColor>(m_window->property(_shadowColor));
}

QPainterPath DPlatformWindowHandle::clipPath() const
{
    return qvariant_cast<QPainterPath>(m_window->property(_clipPath));
}

QRegion DPlatformWindowHandle::frameMask() const
{
    return qvariant_cast<QRegion>(m_window->property(_frameMask));
}

QMargins DPlatformWindowHandle::frameMargins() const
{
    return qvariant_cast<QMargins>(m_window->property(_frameMargins));
}

bool DPlatformWindowHandle::translucentBackground() const
{
    return m_window->property(_translucentBackground).toBool();
}

bool DPlatformWindowHandle::enableSystemResize() const
{
    return m_window->property(_enableSystemResize).toBool();
}

bool DPlatformWindowHandle::enableSystemMove() const
{
    return m_window->property(_enableSystemMove).toBool();
}

bool DPlatformWindowHandle::enableBlurWindow() const
{
    return m_window->property(_enableBlurWindow).toBool();
}

bool DPlatformWindowHandle::autoInputMaskByClipPath() const
{
    return m_window->property(_autoInputMaskByClipPath).toBool();
}

void DPlatformWindowHandle::setWindowRadius(int windowRadius)
{
    m_window->setProperty(_windowRadius, windowRadius);
}

void DPlatformWindowHandle::setBorderWidth(int borderWidth)
{
    m_window->setProperty(_borderWidth, borderWidth);
}

void DPlatformWindowHandle::setBorderColor(const QColor &borderColor)
{
    m_window->setProperty(_borderColor, QVariant::fromValue(borderColor));
}

void DPlatformWindowHandle::setShadowRadius(int shadowRadius)
{
    m_window->setProperty(_shadowRadius, shadowRadius);
}

void DPlatformWindowHandle::setShadowOffset(const QPoint &shadowOffset)
{
    m_window->setProperty(_shadowOffset, shadowOffset);
}

void DPlatformWindowHandle::setShadowColor(const QColor &shadowColor)
{
    m_window->setProperty(_shadowColor, QVariant::fromValue(shadowColor));
}

void DPlatformWindowHandle::setClipPath(const QPainterPath &clipPath)
{
    m_window->setProperty(_clipPath, QVariant::fromValue(clipPath));
}

void DPlatformWindowHandle::setFrameMask(const QRegion &frameMask)
{
    m_window->setProperty(_frameMask, QVariant::fromValue(frameMask));
}

void DPlatformWindowHandle::setTranslucentBackground(bool translucentBackground)
{
    m_window->setProperty(_translucentBackground, translucentBackground);
}

void DPlatformWindowHandle::setEnableSystemResize(bool enableSystemResize)
{
    m_window->setProperty(_enableSystemResize, enableSystemResize);
}

void DPlatformWindowHandle::setEnableSystemMove(bool enableSystemMove)
{
    m_window->setProperty(_enableSystemMove, enableSystemMove);
}

void DPlatformWindowHandle::setEnableBlurWindow(bool enableBlurWindow)
{
    m_window->setProperty(_enableBlurWindow, enableBlurWindow);
}

void DPlatformWindowHandle::setAutoInputMaskByClipPath(bool autoInputMaskByClipPath)
{
    m_window->setProperty(_autoInputMaskByClipPath, autoInputMaskByClipPath);
}

bool DPlatformWindowHandle::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == m_window) {
        if (event->type() == QEvent::DynamicPropertyChange) {
            QDynamicPropertyChangeEvent *e = static_cast<QDynamicPropertyChangeEvent *>(event);

            if (e->propertyName() == _windowRadius) {
                Q_EMIT windowRadiusChanged();
            } else if (e->propertyName() == _borderWidth) {
                Q_EMIT borderWidthChanged();
            } else if (e->propertyName() == _borderColor) {
                Q_EMIT borderColorChanged();
            } else if (e->propertyName() == _shadowRadius) {
                Q_EMIT shadowRadiusChanged();
            } else if (e->propertyName() == _shadowOffset) {
                Q_EMIT shadowOffsetChanged();
            } else if (e->propertyName() == _shadowColor) {
                Q_EMIT shadowColorChanged();
            } else if (e->propertyName() == _clipPath) {
                Q_EMIT clipPathChanged();
            } else if (e->propertyName() == _frameMask) {
                Q_EMIT frameMaskChanged();
            } else if (e->propertyName() == _frameMargins) {
                Q_EMIT frameMarginsChanged();
            } else if (e->propertyName() == _translucentBackground) {
                Q_EMIT translucentBackgroundChanged();
            } else if (e->propertyName() == _enableSystemResize) {
                Q_EMIT enableSystemResizeChanged();
            } else if (e->propertyName() == _enableSystemMove) {
                Q_EMIT enableSystemMoveChanged();
            } else if (e->propertyName() == _enableBlurWindow) {
                Q_EMIT enableBlurWindowChanged();
            } else if (e->propertyName() == _autoInputMaskByClipPath) {
                Q_EMIT autoInputMaskByClipPathChanged();
            }
        }
    }

    return false;
}

DWIDGET_END_NAMESPACE

QT_BEGIN_NAMESPACE
QDebug operator<<(QDebug deg, const DPlatformWindowHandle::WMBlurArea &area)
{
    QDebugStateSaver saver(deg);
    Q_UNUSED(saver)

    deg.setAutoInsertSpaces(true);
    deg << "x:" << area.x
        << "y:" << area.y
        << "width:" << area.width
        << "height:" << area.height
        << "xRadius:" << area.xRadius
        << "yRadius:" << area.yRaduis;

    return deg;
}
QT_END_NAMESPACE
