// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.weblayer_private;

import android.annotation.SuppressLint;
import android.content.Context;
import android.graphics.Rect;
import android.view.View;
import android.view.ViewParent;
import android.widget.FrameLayout;

import org.chromium.base.MathUtils;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.base.annotations.NativeMethods;
import org.chromium.content_public.browser.WebContents;
import org.chromium.content_public.common.BrowserControlsState;
import org.chromium.ui.base.EventOffsetHandler;
import org.chromium.ui.resources.dynamics.ViewResourceAdapter;

/**
 * BrowserControlsContainerView is responsible for positioning and sizing a view from the client
 * that is anchored to the top or bottom of a Browser. BrowserControlsContainerView, uses
 * ViewResourceAdapter that is kept in sync with the contents of the view. ViewResourceAdapter is
 * used to keep a bitmap in sync with the contents of the view. The bitmap is placed in a cc::Layer
 * and the layer is shown while scrolling. ViewResourceAdapter is always kept in sync, as to do
 * otherwise results in a noticeable delay between when the scroll starts the content is available.
 *
 * There are many parts involved in orchestrating scrolling. The key things to know are:
 * . BrowserControlsContainerView (in native code) keeps a cc::Layer that shows a bitmap rendered by
 *   the view. The bitmap is updated anytime the view changes. This is done as otherwise there is a
 *   noticeable delay between when the scroll starts and the bitmap is available.
 * . When scrolling, the cc::Layer for the WebContents and BrowserControlsContainerView is moved.
 * . The size of the WebContents is only changed after the user releases a touch point. Otherwise
 *   the scrollbar bounces around.
 * . WebContentsDelegate::DoBrowserControlsShrinkRendererSize() only changes when the WebContents
 *   size change.
 * . WebContentsGestureStateTracker is responsible for determining when a scroll/touch is underway.
 * . ContentViewRenderView.Delegate is used to adjust the size of the webcontents when the
 *   controls are fully visible (and a scroll is not underway).
 *
 * The flow of this code is roughly:
 * . WebContentsGestureStateTracker generally detects a touch first
 * . TabImpl is notified and caches state.
 * . onTop/BottomControlsChanged() is called. This triggers hiding the real view and calling to
 *   native code to move the cc::Layers.
 * . the move continues.
 * . when the move completes and both WebContentsGestureStateTracker and
 * BrowserControlsContainerView no longer believe a move/gesture/scroll is underway the size of the
 * WebContents is adjusted (if necessary).
 */
@JNINamespace("weblayer")
class BrowserControlsContainerView extends FrameLayout {
    // ID used with ViewResourceAdapter.
    private static final int TOP_CONTROLS_ID = 1001;
    private static final int BOTTOM_CONTROLS_ID = 1002;

    private static final long SYSTEM_UI_VIEWPORT_UPDATE_DELAY_MS = 500;

    private final Delegate mDelegate;
    private final boolean mIsTop;

    private long mNativeBrowserControlsContainerView;

    private ViewResourceAdapter mViewResourceAdapter;

    // Last width/height of mView as sent to the native side.
    private int mLastWidth;
    private int mLastHeight;

    // View from the client.
    private View mView;

    private ContentViewRenderView mContentViewRenderView;
    private WebContents mWebContents;
    // Only created when hosting top-controls.
    private EventOffsetHandler mEventOffsetHandler;

    // Amount page content is offset along the y-axis. This is always 0 for bottom controls (because
    // bottom controls don't offset content). For top controls, a value of 0 means no offset, and
    // positive values indicate a portion of the top-control is shown. This value never goes
    // negative.
    private int mContentOffset;

    // Amount the control is offset along the y-axis from it's fully shown position. For
    // top-controls, the value ranges from 0 (completely shown) to -height (completely hidden). For
    // bottom-controls, the value ranges from 0 (completely shown) to height (completely hidden).
    private int mControlsOffset;

    // The minimum height that the controls should collapse to. Only used for top controls.
    private int mMinHeight;

    // Whether the controls should only expand when the page is scrolled to the top. Only used for
    // top controls.
    private boolean mPinControlsToContentTop;

    // Set to true if changes to the controls height or offset should be animated.
    private boolean mShouldAnimate;

    // Set to true if |mView| is hidden because the user has scrolled or triggered some action such
    // that mView is not visible. While |mView| is not visible if this is true, the bitmap from
    // |mView| may be partially visible.
    private boolean mInScroll;

    // Set to true while we are animating the controls off the screen after removing them.
    private boolean mAnimatingOut;

    private boolean mIsFullscreen;

    // Used to delay processing fullscreen requests.
    private Runnable mSystemUiFullscreenResizeRunnable;

    // Used to  delay updating the image for the layer.
    private final Runnable mRefreshResourceIdRunnable = () -> {
        if (mView == null || mViewResourceAdapter == null) return;
        BrowserControlsContainerViewJni.get().updateControlsResource(
                mNativeBrowserControlsContainerView);
    };

    public interface Delegate {
        /**
         * Requests that the page height be recalculated due to browser controls height changes.
         */
        void refreshPageHeight();

        /**
         * Requests that the browser controls visibility state be changed.
         */
        void setAnimationConstraint(@BrowserControlsState int constraint);
    }

    BrowserControlsContainerView(Context context, ContentViewRenderView contentViewRenderView,
            Delegate delegate, boolean isTop) {
        super(context);
        mDelegate = delegate;
        mIsTop = isTop;
        mContentViewRenderView = contentViewRenderView;
        mNativeBrowserControlsContainerView =
                BrowserControlsContainerViewJni.get().createBrowserControlsContainerView(
                        this, contentViewRenderView.getNativeHandle(), isTop);
    }

    public void setWebContents(WebContents webContents) {
        mWebContents = webContents;
        BrowserControlsContainerViewJni.get().setWebContents(
                mNativeBrowserControlsContainerView, webContents);

        cancelDelayedFullscreenRunnable();
        if (mWebContents == null) return;
        processFullscreenChanged(mWebContents.isFullscreenForCurrentTab());
    }

    public void destroy() {
        setAnimationsEnabled(false);
        setView(null);
        BrowserControlsContainerViewJni.get().deleteBrowserControlsContainerView(
                mNativeBrowserControlsContainerView);
        cancelDelayedFullscreenRunnable();
    }

    public long getNativeHandle() {
        return mNativeBrowserControlsContainerView;
    }

    public EventOffsetHandler getEventOffsetHandler() {
        assert mIsTop;
        if (mEventOffsetHandler == null) {
            mEventOffsetHandler =
                    new EventOffsetHandler(new EventOffsetHandler.EventOffsetHandlerDelegate() {
                        @Override
                        public float getTop() {
                            return mContentOffset;
                        }

                        @Override
                        public void setCurrentTouchEventOffsets(float top) {
                            if (mWebContents != null) {
                                mWebContents.getEventForwarder().setCurrentTouchEventOffsets(
                                        0, top);
                            }
                        }
                    });
        }
        return mEventOffsetHandler;
    }

    /**
     * Returns the amount of vertical space to take away from the contents.
     */
    public int getContentHeightDelta() {
        if (mView == null) return 0;
        return mIsTop ? mContentOffset : mLastHeight - mControlsOffset;
    }

    /**
     * Returns true if the browser control is visible to the user.
     */
    public boolean isControlVisible() {
        // Don't check the visibility of the View itself as it's hidden while scrolling.
        return mView != null && Math.abs(mControlsOffset) != mLastHeight;
    }

    public void setAnimationsEnabled(boolean animationsEnabled) {
        assert mIsTop;
        mShouldAnimate = animationsEnabled;
    }

    /**
     * Returns true if the controls are completely expanded or completely collapsed.
     * "Completely collapsed" does not necessarily mean hidden; the controls could be at their min
     * height, in which case this would return true. A return value of false indicates the controls
     * are being moved.
     */
    public boolean isCompletelyExpandedOrCollapsed() {
        return mControlsOffset == 0 || Math.abs(mControlsOffset) == mLastHeight - mMinHeight;
    }

    /**
     * Sets the view from the client.
     */
    public void setView(View view) {
        if (mView == view) return;

        if (mView != null && mView.getParent() == this) removeView(mView);
        mView = view;

        if (mView == null) {
            // If we're animating out the old view, leave the cc::Layer in place so it's visible
            // during the animation, and set our visibility to HIDDEN, which will cause
            // BrowserControlsOffsetManager to start an animation off the screen. getMinHeight()
            // will return 0 while mAnimatingOut is true, so call reportHeightChange() to tell the
            // renderer to grab the potentially new height.
            if (mShouldAnimate && mControlsOffset != -mLastHeight) {
                assert mIsTop; // mShouldAnimate should only be true for top controls.
                mAnimatingOut = true;
                reportHeightChange();
                mDelegate.setAnimationConstraint(BrowserControlsState.HIDDEN);
                mDelegate.refreshPageHeight();
            } else {
                destroyLayer();
            }
            return;
        }

        mAnimatingOut = false;
        destroyLayer();
        addView(view,
                new FrameLayout.LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.WRAP_CONTENT,
                        FrameLayout.LayoutParams.UNSPECIFIED_GRAVITY));
        // We always want to hide the real controls so they don't flash for a frame before we've
        // figured out where to position them.
        removeCallbacks(this::showControls);
        hideControls();
        mDelegate.setAnimationConstraint(BrowserControlsState.BOTH);
    }

    public View getView() {
        return mView;
    }

    /**
     * Sets the minimum height the controls can collapse to.
     * Only valid for top controls.
     */
    public void setMinHeight(int minHeight) {
        assert mIsTop;
        if (mMinHeight == minHeight) return;
        mMinHeight = minHeight;
        reportHeightChange();
    }

    /**
     * Sets whether the controls should only expand at the top of the page contents.
     * Only valid for top controls.
     */
    public void setPinControlsToContentTop(boolean pinToContentTop) {
        mPinControlsToContentTop = pinToContentTop;
    }

    /**
     * Called from ViewAndroidDelegate, see it for details.
     */
    public void onOffsetsChanged(int controlsOffsetY, int contentOffsetY) {
        // Delete the cc::Layer if we reached the end of the animation off the screen.
        if (mAnimatingOut && controlsOffsetY == -mLastHeight) {
            mAnimatingOut = false;
            destroyLayer();
            reportHeightChange();
            // Request a layout so onLayout can update the saved dimensions now that the
            // layer has finished animating.
            requestLayout();
            return;
        }

        if (mIsFullscreen) return;
        setControlsOffset(controlsOffsetY, contentOffsetY);
        if (controlsOffsetY == 0
                || (mIsTop && getMinHeight() > 0
                        && controlsOffsetY == -mLastHeight + getMinHeight())) {
            finishScroll();
        } else if (!mInScroll) {
            prepareForScroll();
        }
    }

    @SuppressLint("NewApi") // Used on O+, invalidateChildInParent used for previous versions.
    @Override
    public void onDescendantInvalidated(View child, View target) {
        super.onDescendantInvalidated(child, target);
        invalidateViewResourceAdapter();
    }

    @Override
    public ViewParent invalidateChildInParent(int[] location, Rect dirty) {
        invalidateViewResourceAdapter();
        return super.invalidateChildInParent(location, dirty);
    }

    @Override
    protected void onLayout(boolean changed, int left, int top, int right, int bottom) {
        super.onLayout(changed, left, top, right, bottom);
        if (mAnimatingOut) return;
        int width = right - left;
        int height = bottom - top;
        boolean heightChanged = height != mLastHeight;
        if (!heightChanged && width == mLastWidth) return;

        int prevHeight = mLastHeight;
        mLastWidth = width;
        mLastHeight = height;
        if (mLastWidth > 0 && mLastHeight > 0 && mViewResourceAdapter == null) {
            createAdapterAndLayer();
            if (prevHeight == 0) {
                assert heightChanged;
                // If there wasn't a View before (or it had 0 height), move the new View off the
                // screen until we know where to position it.
                moveControlsOffScreen();
            }
        } else if (mViewResourceAdapter != null) {
            BrowserControlsContainerViewJni.get().setControlsSize(
                    mNativeBrowserControlsContainerView, mLastWidth, mLastHeight);
        }
        if (heightChanged) reportHeightChange();
    }

    @Override
    public void onDetachedFromWindow() {
        super.onDetachedFromWindow();
        // Cancel the runnable when detached as calls to removeCallback() after this completes will
        // attempt to remove from the wrong handler.
        cancelDelayedFullscreenRunnable();
    }

    private void cancelDelayedFullscreenRunnable() {
        if (mSystemUiFullscreenResizeRunnable == null) return;
        removeCallbacks(mSystemUiFullscreenResizeRunnable);
        mSystemUiFullscreenResizeRunnable = null;
    }

    /**
     * Triggers copying the contents of mView to the offscreen buffer.
     */
    private void invalidateViewResourceAdapter() {
        if (mViewResourceAdapter == null || mView.getVisibility() != View.VISIBLE) return;
        mViewResourceAdapter.invalidate(null);
        removeCallbacks(mRefreshResourceIdRunnable);
        postOnAnimation(mRefreshResourceIdRunnable);
    }

    /**
     * Creates mViewResourceAdapter and the layer showing a copy of mView.
     */
    private void createAdapterAndLayer() {
        assert mViewResourceAdapter == null;
        assert mView != null;
        mViewResourceAdapter = new ViewResourceAdapter(mView);
        mContentViewRenderView.getResourceManager().getDynamicResourceLoader().registerResource(
                getResourceId(), mViewResourceAdapter);
        // It's important that the layer is created immediately and always kept in sync with the
        // View. Creating the layer only when needed results in a noticeable delay between when
        // the layer is created and actually shown. Chrome for Android does the same thing.
        BrowserControlsContainerViewJni.get().createControlsLayer(
                mNativeBrowserControlsContainerView, getResourceId());
        BrowserControlsContainerViewJni.get().setControlsSize(
                mNativeBrowserControlsContainerView, mLastWidth, mLastHeight);
    }

    /**
     * Destroys the cc::Layer containing the bitmap copy of the View.
     */
    private void destroyLayer() {
        if (mViewResourceAdapter == null) return;
        // TODO: need some sort of destroy to drop reference.
        mViewResourceAdapter = null;
        BrowserControlsContainerViewJni.get().deleteControlsLayer(
                mNativeBrowserControlsContainerView);
        mContentViewRenderView.getResourceManager().getDynamicResourceLoader().unregisterResource(
                getResourceId());
    }

    private void setControlsOffset(int controlsOffsetY, int contentOffsetY) {
        // This function is called asynchronously from the gpu, and may be out of sync with the
        // current values.
        if (mIsTop) {
            // Don't snap to min-height because the controls could be animating in from a
            // previously lower min-height.
            mControlsOffset = MathUtils.clamp(controlsOffsetY, -mLastHeight, 0);
        } else {
            mControlsOffset = MathUtils.clamp(controlsOffsetY, 0, mLastHeight);
        }
        mContentOffset = MathUtils.clamp(contentOffsetY, 0, mLastHeight);

        if (isCompletelyExpandedOrCollapsed()) {
            mDelegate.refreshPageHeight();
        }
        if (mIsTop) {
            BrowserControlsContainerViewJni.get().setTopControlsOffset(
                    mNativeBrowserControlsContainerView, mContentOffset);
        } else {
            BrowserControlsContainerViewJni.get().setBottomControlsOffset(
                    mNativeBrowserControlsContainerView);
        }
    }

    private void reportHeightChange() {
        if (mWebContents != null) {
            mWebContents.notifyBrowserControlsHeightChanged();
        }
    }

    private void prepareForScroll() {
        mInScroll = true;
        if (BrowserControlsContainerViewJni.get().shouldDelayVisibilityChange()) {
            mContentViewRenderView.postOnAnimation(this::hideControls);
        } else {
            hideControls();
        }
    }

    private void finishScroll() {
        mInScroll = false;
        if (BrowserControlsContainerViewJni.get().shouldDelayVisibilityChange()) {
            mContentViewRenderView.postOnAnimation(this::showControls);
        } else {
            showControls();
        }
    }

    private void hideControls() {
        if (mView != null) mView.setVisibility(View.INVISIBLE);
    }

    private void showControls() {
        if (mView != null) {
            if (mIsTop) mView.setTranslationY(mControlsOffset);
            mView.setVisibility(View.VISIBLE);
        }
    }

    @CalledByNative
    /* package */ boolean shouldAnimateBrowserControlsHeightChanges() {
        return mShouldAnimate;
    }

    @CalledByNative
    private int getControlsOffset() {
        return mControlsOffset;
    }

    @CalledByNative
    private int getMinHeight() {
        if (mAnimatingOut) return 0;
        return Math.min(mLastHeight, mMinHeight);
    }

    @CalledByNative
    private boolean shouldPinControlsToContentTop() {
        return mPinControlsToContentTop;
    }

    @CalledByNative
    private void didToggleFullscreenModeForTab(final boolean isFullscreen) {
        // Delay hiding until after the animation. This comes from Chrome code.
        if (mSystemUiFullscreenResizeRunnable != null) {
            removeCallbacks(mSystemUiFullscreenResizeRunnable);
        }
        mSystemUiFullscreenResizeRunnable = () -> processFullscreenChanged(isFullscreen);
        long delay = isFullscreen ? SYSTEM_UI_VIEWPORT_UPDATE_DELAY_MS : 0;
        postDelayed(mSystemUiFullscreenResizeRunnable, delay);
    }

    private void processFullscreenChanged(boolean isFullscreen) {
        mSystemUiFullscreenResizeRunnable = null;
        if (mIsFullscreen == isFullscreen) return;
        mIsFullscreen = isFullscreen;
        if (mIsFullscreen) {
            mAnimatingOut = false;
            hideControls();
            moveControlsOffScreen();
        } else {
            showControls();
            setControlsOffset(0, mIsTop ? mLastHeight : 0);
        }
    }

    private void moveControlsOffScreen() {
        setControlsOffset(mIsTop ? -mLastHeight : mLastHeight, 0);
    }

    private int getResourceId() {
        return mIsTop ? TOP_CONTROLS_ID : BOTTOM_CONTROLS_ID;
    }

    @NativeMethods
    interface Natives {
        long createBrowserControlsContainerView(
                BrowserControlsContainerView view, long nativeContentViewRenderView, boolean isTop);
        void deleteBrowserControlsContainerView(long nativeBrowserControlsContainerView);
        void createControlsLayer(long nativeBrowserControlsContainerView, int id);
        void deleteControlsLayer(long nativeBrowserControlsContainerView);
        void setTopControlsOffset(long nativeBrowserControlsContainerView, int contentOffsetY);
        void setBottomControlsOffset(long nativeBrowserControlsContainerView);
        void setControlsSize(long nativeBrowserControlsContainerView, int width, int height);
        void updateControlsResource(long nativeBrowserControlsContainerView);
        void setWebContents(long nativeBrowserControlsContainerView, WebContents webContents);
        boolean shouldDelayVisibilityChange();
    }
}
