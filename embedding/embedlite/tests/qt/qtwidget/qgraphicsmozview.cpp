/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-*/
/* vim: set ts=2 sw=2 et tw=79: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#define LOG_COMPONENT "QGraphicsMozView"

#include <QStyleOptionGraphicsItem>
#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include <QTimer>
#include <QTime>
#include <QtOpenGL/QGLContext>
#include "EmbedQtKeyUtils.h"

#include "mozilla-config.h"
#include "qgraphicsmozview.h"
#include "qmozcontext.h"
#include "InputData.h"
#include "mozilla/embedlite/EmbedLog.h"
#include "mozilla/embedlite/EmbedLiteView.h"
#include "mozilla/embedlite/EmbedLiteApp.h"

using namespace mozilla;
using namespace mozilla::embedlite;

class QGraphicsMozViewPrivate : public EmbedLiteViewListener {
public:
    QGraphicsMozViewPrivate(QGraphicsMozView* view)
      : q(view)
      , mContext(NULL)
      , mView(NULL)
      , mViewInitialized(false)
      , mPendingTouchEvent(false)
      , mProgress(100)
      , mCanGoBack(false)
      , mCanGoForward(false)
      , mIsLoading(false)
      , mLastIsGoodRotation(true)
    {
    }
    virtual ~QGraphicsMozViewPrivate() {}

    void ReceiveInputEvent(const mozilla::InputData& event);
    void touchEvent(QTouchEvent* event);
    void UpdateViewSize()
    {
        if (mViewInitialized) {
            const QGLContext* ctx = QGLContext::currentContext();
            if (ctx && ctx->device()) {
                QRectF r(0, 0, ctx->device()->width(), ctx->device()->height());
                r = q->mapRectToScene(r);
                mView->SetGLViewPortSize(r.width(), r.height());
            }
            mView->SetViewSize(mSize.width(), mSize.height());
        }
    }
    virtual void ViewInitialized() {
        mViewInitialized = true;
        UpdateViewSize();
        Q_EMIT q->viewInitialized();
        Q_EMIT q->navigationHistoryChanged();
    }
    virtual void SetBackgroundColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
        mBgColor = QColor(r, g, b, a);
    }
    virtual bool Invalidate() {
        q->update();
        return true;
    }
    virtual void OnTitleChanged(const PRUnichar* aTitle) {
        mTitle = QString((const QChar*)aTitle);
        Q_EMIT q->titleChanged();
    }
    virtual void OnLocationChanged(const char* aLocation, bool aCanGoBack, bool aCanGoForward) {
        mLocation = QString(aLocation);
        if (mCanGoBack != aCanGoBack || mCanGoForward != aCanGoForward) {
            mCanGoBack = aCanGoBack;
            mCanGoForward = aCanGoForward;
            Q_EMIT q->navigationHistoryChanged();
        }
        Q_EMIT q->urlChanged();
    }
    virtual void OnLoadProgress(int32_t aProgress, int32_t aCurTotal, int32_t aMaxTotal) {
        mProgress = aProgress;
        Q_EMIT q->loadProgressChanged();
    }
    virtual void OnLoadStarted(const char* aLocation) {
        if (mLocation != aLocation) {
            mLocation = aLocation;
            Q_EMIT q->urlChanged();
        }
        if (!mIsLoading) {
            mIsLoading = true;
            Q_EMIT q->loadingChanged();
        }
    }
    virtual void OnLoadFinished(void) {
        if (mIsLoading) {
            mIsLoading = false;
            Q_EMIT q->loadingChanged();
        }
    }

    // View finally destroyed and deleted
    virtual void Destroyed() {
        LOGT();
        Q_EMIT q->viewDestroyed();
    }
    virtual void RecvAsyncMessage(const char* aMessage, const char* aData) {
        LOGT();
        Q_EMIT q->recvAsyncMessage(aMessage, aData);
    }
    virtual char* RecvSyncMessage(const char* aMessage, const char* aData) {
        LOGT();
        QSyncMessageResponse response;
        Q_EMIT q->recvSyncMessage(aMessage, aData, &response);
        LOGT("msg:%s, response:%s", aMessage, response.getMessage().toUtf8().data());
        return strdup(response.getMessage().toUtf8().data());
    }

    virtual void OnLoadRedirect(void) {
        LOGT();
        Q_EMIT q->loadRedirect();
    }

    virtual void OnSecurityChanged(const char* aStatus, unsigned int aState) {
        LOGT();
        Q_EMIT q->securityChanged(aStatus, aState);
    }
    virtual void OnFirstPaint(int32_t aX, int32_t aY) {
        LOGT();
        Q_EMIT q->firstPaint(aX, aY);
    }
    virtual void OnContentLoaded(const PRUnichar* aDocURI) {
        LOGT();
        Q_EMIT q->contentLoaded(QString((QChar*)aDocURI));
    }
    virtual void OnLinkAdded(const PRUnichar* aHref, const PRUnichar* aCharset, const PRUnichar* aTitle, const PRUnichar* aRel, const PRUnichar* aSizes, const PRUnichar* aType) { LOGT(); }
    virtual void OnWindowOpenClose(const PRUnichar* aType) { LOGT(); }
    virtual void OnPopupBlocked(const char* aSpec, const char* aCharset, const PRUnichar* aPopupFeatures, const PRUnichar* aPopupWinName) { LOGT(); }
    virtual void OnPageShowHide(const PRUnichar* aType, bool aPersisted) { LOGT(); }
    virtual void OnScrolledAreaChanged(unsigned int aWidth, unsigned int aHeight) { LOGT(); }
    virtual void OnScrollChanged(int32_t offSetX, int32_t offSetY) { }
    virtual void OnObserve(const char* aTopic, const PRUnichar* aData) {
        LOGT();
        Q_EMIT q->observeNotification(aTopic, QString((QChar*)aData));
    }
    virtual void SetFirstPaintViewport(const nsIntPoint& aOffset, float aZoom,
                                       const nsIntRect& aPageRect, const gfxRect& aCssPageRect) { LOGT(); }
    virtual void SyncViewportInfo(const nsIntRect& aDisplayPort,
                                  float aDisplayResolution, bool aLayersUpdated,
                                  nsIntPoint& aScrollOffset, float& aScaleX, float& aScaleY) { LOGT(); }
    virtual void SetPageRect(const gfxRect& aCssPageRect) { LOGT(); }

    QGraphicsMozView* q;
    QMozContext* mContext;
    EmbedLiteView* mView;
    bool mViewInitialized;
    QColor mBgColor;
    QImage mTempBufferImage;
    QSize mSize;
    QTime mTouchTime;
    bool mPendingTouchEvent;
    QTime mPanningTime;
    QString mLocation;
    QString mTitle;
    int mProgress;
    bool mCanGoBack;
    bool mCanGoForward;
    bool mIsLoading;
    bool mLastIsGoodRotation;
    QString mLastPreEdit;
};

QGraphicsMozView::QGraphicsMozView(QGraphicsItem* parent)
    : QGraphicsWidget(parent)
    , d(new QGraphicsMozViewPrivate(this))
{
    LOGT();
    setFlag(QGraphicsItem::ItemUsesExtendedStyleOption, true);
    setAcceptDrops(true);
    setAcceptTouchEvents(true);
    setFocusPolicy(Qt::StrongFocus);
    setFlag(QGraphicsItem::ItemClipsChildrenToShape, true);

    setFlag(QGraphicsItem::ItemAcceptsInputMethod, true);

    setAcceptedMouseButtons(Qt::LeftButton | Qt::RightButton | Qt::MiddleButton);
    setFlag(QGraphicsItem::ItemIsFocusScope, true);
    setFlag(QGraphicsItem::ItemIsFocusable, true);

    d->mContext = QMozContext::GetInstance();
    if (QGLContext::currentContext()) {
        d->mContext->GetApp()->SetIsAccelerated(true);
    }
    if (!d->mContext->initialized()) {
        connect(d->mContext, SIGNAL(onInitialized()), this, SLOT(onInitialized()));
    } else {
        QTimer::singleShot(0, this, SLOT(onInitialized()));
    }
}

QGraphicsMozView::~QGraphicsMozView()
{
    delete d;
}

void
QGraphicsMozView::onInitialized()
{
    LOGT();
    d->mView = d->mContext->GetApp()->CreateView();
    d->mView->SetListener(d);
}

void
QGraphicsMozView::paint(QPainter* painter, const QStyleOptionGraphicsItem* opt, QWidget*)
{
    QRect r = opt ? opt->exposedRect.toRect() : boundingRect().toRect();
    if (d->mViewInitialized) {
        QMatrix affine = painter->transform().toAffine();
        gfxMatrix matr(affine.m11(), affine.m12(), affine.m21(), affine.m22(), affine.dx(), affine.dy());
        bool changedState = d->mLastIsGoodRotation != matr.PreservesAxisAlignedRectangles();
        d->mLastIsGoodRotation = matr.PreservesAxisAlignedRectangles();
        if (d->mContext->GetApp()->IsAccelerated()) {
            d->mView->SetGLViewTransform(matr);
            if (changedState) {
                d->UpdateViewSize();
            }
            if (d->mLastIsGoodRotation) {
                painter->fillRect(r, d->mBgColor);
                d->mView->RenderGL();
            }
        } else {
            if (d->mTempBufferImage.isNull() || d->mTempBufferImage.width() != r.width() || d->mTempBufferImage.height() != r.height()) {
                d->mTempBufferImage = QImage(r.size(), QImage::Format_RGB32);
            }
            {
                QPainter imgPainter(&d->mTempBufferImage);
                imgPainter.fillRect(r, d->mBgColor);
            }
            d->mView->RenderToImage(d->mTempBufferImage.bits(), d->mTempBufferImage.width(),
                                    d->mTempBufferImage.height(), d->mTempBufferImage.bytesPerLine(),
                                    d->mTempBufferImage.depth());
            painter->drawImage(QPoint(0, 0), d->mTempBufferImage);
        }
    } else {
        painter->fillRect(r, Qt::white);
    }
}

/*! \reimp
*/
QSizeF QGraphicsMozView::sizeHint(Qt::SizeHint which, const QSizeF& constraint) const
{
    if (which == Qt::PreferredSize)
        return QSizeF(800, 600); // ###
    return QGraphicsWidget::sizeHint(which, constraint);
}

/*! \reimp
*/
void QGraphicsMozView::setGeometry(const QRectF& rect)
{
    QGraphicsWidget::setGeometry(rect);

    // NOTE: call geometry() as setGeometry ensures that
    // the geometry is within legal bounds (minimumSize, maximumSize)
    d->mSize = geometry().size().toSize();
    d->UpdateViewSize();
}

QUrl QGraphicsMozView::url() const
{
    return QUrl(d->mLocation);
}

void QGraphicsMozView::setUrl(const QUrl& url)
{
    if (url.isEmpty())
        return;

    if (!d->mViewInitialized) {
        return;
    }
    LOGT("url: %s", url.toString().toUtf8().data());
    d->mView->LoadURL(url.toString().toUtf8().data());
}

void QGraphicsMozView::load(const QString& url)
{
    if (url.isEmpty())
        return;

    if (!d->mViewInitialized) {
        return;
    }
    LOGT("url: %s", url.toUtf8().data());
    d->mView->LoadURL(QUrl::fromUserInput(url).toString().toUtf8().data());
}

QString QGraphicsMozView::title() const
{
    return d->mTitle;
}

int QGraphicsMozView::loadProgress() const
{
    return d->mProgress;
}

bool QGraphicsMozView::canGoBack() const
{
    return d->mCanGoBack;
}

bool QGraphicsMozView::canGoForward() const
{
    return d->mCanGoForward;
}

bool QGraphicsMozView::loading() const
{
    return d->mIsLoading;
}

void QGraphicsMozView::loadHtml(const QString& html, const QUrl& baseUrl)
{
    LOGT();
}

void QGraphicsMozView::goBack()
{
    LOGT();
}

void QGraphicsMozView::goForward()
{
    LOGT();
}

void QGraphicsMozView::stop()
{
    LOGT();
}

void QGraphicsMozView::reload()
{
    LOGT();
}

bool QGraphicsMozView::event(QEvent* event)
{
    switch (event->type()) {
    case QEvent::TouchBegin:
    case QEvent::TouchEnd:
    case QEvent::TouchUpdate: {
        d->touchEvent(static_cast<QTouchEvent*>(event));
        return true;
    }
    case QEvent::Show: {
        printf(">>>>>>Func:%s::%d Event Show\n", __PRETTY_FUNCTION__, __LINE__);
        break;
    }
    case QEvent::Hide: {
        printf(">>>>>>Func:%s::%d Event Hide\n", __PRETTY_FUNCTION__, __LINE__);
        break;
    }
    default:
        break;
    }

    // Here so that it can be reimplemented without breaking ABI.
    return QGraphicsWidget::event(event);
}

void QGraphicsMozViewPrivate::touchEvent(QTouchEvent* event)
{
    mPendingTouchEvent = event->type() == QEvent::TouchEnd ? false : true;
    // Always accept the QTouchEvent so that we'll receive also TouchUpdate and TouchEnd events
    event->setAccepted(true);
    if (event->type() == QEvent::TouchBegin) {
        q->forceActiveFocus();
        mTouchTime.restart();
    }

    MultiTouchInput meventStart(MultiTouchInput::MULTITOUCH_START, mTouchTime.elapsed());
    MultiTouchInput meventMove(MultiTouchInput::MULTITOUCH_MOVE, mTouchTime.elapsed());
    MultiTouchInput meventEnd(MultiTouchInput::MULTITOUCH_END, mTouchTime.elapsed());
    for (int i = 0; i < event->touchPoints().size(); ++i) {
        const QTouchEvent::TouchPoint& pt = event->touchPoints().at(i);
        nsIntPoint nspt(pt.pos().x(), pt.pos().y());
        switch (pt.state())
        {
            case Qt::TouchPointPressed: {
                meventStart.mTouches.AppendElement(SingleTouchData(pt.id(),
                                                   nspt,
                                                   nsIntPoint(1, 1),
                                                   180.0f,
                                                   1.0f));
                break;
            }
            case Qt::TouchPointReleased: {
                meventEnd.mTouches.AppendElement(SingleTouchData(pt.id(),
                                                 nspt,
                                                 nsIntPoint(1, 1),
                                                 180.0f,
                                                 1.0f));
                break;
            }
            case Qt::TouchPointMoved: {
                meventMove.mTouches.AppendElement(SingleTouchData(pt.id(),
                                                  nspt,
                                                  nsIntPoint(1, 1),
                                                  180.0f,
                                                  1.0f));
                break;
            }
            default:
                break;
        }
    }
    if (meventStart.mTouches.Length())
        ReceiveInputEvent(meventStart);
    if (meventMove.mTouches.Length())
        ReceiveInputEvent(meventMove);
    if (meventEnd.mTouches.Length())
        ReceiveInputEvent(meventEnd);
}

void QGraphicsMozViewPrivate::ReceiveInputEvent(const InputData& event)
{
    if (mViewInitialized) {
        mView->ReceiveInputEvent(event);
    }
}

void QGraphicsMozView::mouseMoveEvent(QGraphicsSceneMouseEvent* e)
{
    if (d->mViewInitialized && !d->mPendingTouchEvent) {
        const bool accepted = e->isAccepted();
        MultiTouchInput event(MultiTouchInput::MULTITOUCH_MOVE, d->mPanningTime.elapsed());
        event.mTouches.AppendElement(SingleTouchData(0,
                                     nsIntPoint(e->pos().x(), e->pos().y()),
                                     nsIntPoint(1, 1),
                                     180.0f,
                                     1.0f));
        d->ReceiveInputEvent(event);
        e->setAccepted(accepted);
    }

    if (!e->isAccepted())
        QGraphicsItem::mouseMoveEvent(e);
}

void QGraphicsMozView::mousePressEvent(QGraphicsSceneMouseEvent* e)
{
    d->mPanningTime.restart();
    forceActiveFocus();
    if (d->mViewInitialized && !d->mPendingTouchEvent) {
        const bool accepted = e->isAccepted();
        MultiTouchInput event(MultiTouchInput::MULTITOUCH_START, d->mPanningTime.elapsed());
        event.mTouches.AppendElement(SingleTouchData(0,
                                     nsIntPoint(e->pos().x(), e->pos().y()),
                                     nsIntPoint(1, 1),
                                     180.0f,
                                     1.0f));
        d->ReceiveInputEvent(event);
        e->setAccepted(accepted);
    }

    if (!e->isAccepted())
        QGraphicsItem::mouseMoveEvent(e);
}

void QGraphicsMozView::mouseReleaseEvent(QGraphicsSceneMouseEvent* e)
{
    if (d->mViewInitialized && !d->mPendingTouchEvent) {
        const bool accepted = e->isAccepted();
        MultiTouchInput event(MultiTouchInput::MULTITOUCH_END, d->mPanningTime.elapsed());
        event.mTouches.AppendElement(SingleTouchData(0,
                                     nsIntPoint(e->pos().x(), e->pos().y()),
                                     nsIntPoint(1, 1),
                                     180.0f,
                                     1.0f));
        d->ReceiveInputEvent(event);
        e->setAccepted(accepted);
    }

    if (!e->isAccepted())
        QGraphicsItem::mouseMoveEvent(e);
}

void QGraphicsMozView::forceActiveFocus()
{
    QGraphicsItem *parent = parentItem();
    while (parent) {
        if (parent->flags() & QGraphicsItem::ItemIsFocusScope)
            parent->setFocus(Qt::OtherFocusReason);
        parent = parent->parentItem();
    }

    setFocus(Qt::OtherFocusReason);
    if (d->mViewInitialized) {
        d->mView->SetIsActive(true);
    }
}

void QGraphicsMozView::inputMethodEvent(QInputMethodEvent* aEvent)
{
    LOGT("cStr:%s, preStr:%s, replLen:%i, replSt:%i", aEvent->commitString().toUtf8().data(), aEvent->preeditString().toUtf8().data(), aEvent->replacementLength(), aEvent->replacementStart());
    if (d->mViewInitialized) {
        d->mView->SendTextEvent(aEvent->commitString().toUtf8().data(), aEvent->preeditString().toUtf8().data());
    }
}

void QGraphicsMozView::keyPressEvent(QKeyEvent* event)
{
    if (!d->mViewInitialized)
        return;

    LOGT();
    int32_t gmodifiers = MozKey::QtModifierToDOMModifier(event->modifiers());
    int32_t domKeyCode = MozKey::QtKeyCodeToDOMKeyCode(event->key(), event->modifiers());
    int32_t charCode = 0;
    if (event->text().length() && event->text()[0].isPrint())
        charCode = (int32_t)event->text()[0].unicode();
    d->mView->SendKeyPress(domKeyCode, gmodifiers, charCode);
}

void QGraphicsMozView::keyReleaseEvent(QKeyEvent* event)
{
    if (!d->mViewInitialized)
        return;

    LOGT();
    int32_t gmodifiers = MozKey::QtModifierToDOMModifier(event->modifiers());
    int32_t domKeyCode = MozKey::QtKeyCodeToDOMKeyCode(event->key(), event->modifiers());
    int32_t charCode = 0;
    if (event->text().length() && event->text()[0].isPrint())
        charCode = (int32_t)event->text()[0].unicode();
    d->mView->SendKeyRelease(domKeyCode, gmodifiers, charCode);
}

QVariant
QGraphicsMozView::inputMethodQuery(Qt::InputMethodQuery aQuery) const
{
    if (aQuery == 10001) {
        LOGT("VisualizationPriorityQuery");
    }
    if (aQuery == 10003) {
        LOGT("ImCorrectionEnabledQuery");
    }
    if (aQuery == 10004) {
        LOGT("ImModeQuery");
    }
    if (aQuery == 10005) {
        LOGT("InputMethodToolbarIdQuery");
    }
    if (aQuery == 10006) {
        LOGT("InputMethodAttributeExtensionIdQuery");
    }
    if (aQuery == 10007) {
        LOGT("InputMethodToolbarQuery");
    }
    if (aQuery == Qt::ImCurrentSelection) {
        LOGT("Qt::ImCurrentSelection");
    }
    if (aQuery == Qt::ImMicroFocus) {
        LOGT("Qt::ImMicroFocus");
    }
    if (aQuery == Qt::ImSurroundingText) {
        LOGT("Qt::ImSurroundingText");
    }
    if (aQuery == Qt::ImCursorPosition) {
        LOGT("Qt::ImCursorPosition");
    }
    if (aQuery == Qt::ImAnchorPosition) {
        LOGT("Qt::ImAnchorPosition");
    }

    return QVariant(0);
}
