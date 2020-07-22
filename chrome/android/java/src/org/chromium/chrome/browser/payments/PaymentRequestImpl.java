// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.payments;

import android.content.Context;
import android.os.Handler;
import android.text.TextUtils;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.VisibleForTesting;
import androidx.collection.ArrayMap;

import org.chromium.base.Callback;
import org.chromium.base.LocaleUtils;
import org.chromium.base.Log;
import org.chromium.base.metrics.RecordHistogram;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ChromeActivity;
import org.chromium.chrome.browser.ChromeTabbedActivity;
import org.chromium.chrome.browser.autofill.PersonalDataManager;
import org.chromium.chrome.browser.autofill.PersonalDataManager.AutofillProfile;
import org.chromium.chrome.browser.autofill.PersonalDataManager.NormalizedAddressRequestDelegate;
import org.chromium.chrome.browser.compositor.layouts.EmptyOverviewModeObserver;
import org.chromium.chrome.browser.compositor.layouts.OverviewModeBehavior;
import org.chromium.chrome.browser.compositor.layouts.OverviewModeBehavior.OverviewModeObserver;
import org.chromium.chrome.browser.payments.handler.PaymentHandlerCoordinator;
import org.chromium.chrome.browser.payments.handler.PaymentHandlerCoordinator.PaymentHandlerUiObserver;
import org.chromium.chrome.browser.payments.handler.PaymentHandlerCoordinator.PaymentHandlerWebContentsObserver;
import org.chromium.chrome.browser.payments.minimal.MinimalUICoordinator;
import org.chromium.chrome.browser.payments.ui.ContactDetailsSection;
import org.chromium.chrome.browser.payments.ui.LineItem;
import org.chromium.chrome.browser.payments.ui.PaymentInformation;
import org.chromium.chrome.browser.payments.ui.PaymentRequestSection.OptionSection.FocusChangedObserver;
import org.chromium.chrome.browser.payments.ui.PaymentRequestUI;
import org.chromium.chrome.browser.payments.ui.PaymentRequestUI.SelectionResult;
import org.chromium.chrome.browser.payments.ui.PaymentUIsManager;
import org.chromium.chrome.browser.payments.ui.SectionInformation;
import org.chromium.chrome.browser.payments.ui.ShoppingCart;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.settings.SettingsLauncher;
import org.chromium.chrome.browser.settings.SettingsLauncherImpl;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tab.TabSelectionType;
import org.chromium.chrome.browser.tabmodel.EmptyTabModelSelectorObserver;
import org.chromium.chrome.browser.tabmodel.TabModel;
import org.chromium.chrome.browser.tabmodel.TabModelObserver;
import org.chromium.chrome.browser.tabmodel.TabModelSelector;
import org.chromium.chrome.browser.tabmodel.TabModelSelectorObserver;
import org.chromium.chrome.browser.ui.favicon.FaviconHelper;
import org.chromium.components.autofill.Completable;
import org.chromium.components.autofill.EditableOption;
import org.chromium.components.browser_ui.bottomsheet.BottomSheetControllerProvider;
import org.chromium.components.embedder_support.util.UrlConstants;
import org.chromium.components.page_info.CertificateChainHelper;
import org.chromium.components.payments.AbortReason;
import org.chromium.components.payments.ComponentPaymentRequestImpl;
import org.chromium.components.payments.ComponentPaymentRequestImpl.ComponentPaymentRequestDelegate;
import org.chromium.components.payments.CurrencyFormatter;
import org.chromium.components.payments.ErrorMessageUtil;
import org.chromium.components.payments.ErrorStrings;
import org.chromium.components.payments.Event;
import org.chromium.components.payments.JourneyLogger;
import org.chromium.components.payments.MethodStrings;
import org.chromium.components.payments.NotShownReason;
import org.chromium.components.payments.OriginSecurityChecker;
import org.chromium.components.payments.PackageManagerDelegate;
import org.chromium.components.payments.PayerData;
import org.chromium.components.payments.PaymentApp;
import org.chromium.components.payments.PaymentAppFactoryParams;
import org.chromium.components.payments.PaymentAppType;
import org.chromium.components.payments.PaymentDetailsConverter;
import org.chromium.components.payments.PaymentDetailsUpdateServiceHelper;
import org.chromium.components.payments.PaymentFeatureList;
import org.chromium.components.payments.PaymentHandlerHost;
import org.chromium.components.payments.PaymentOptionsUtils;
import org.chromium.components.payments.PaymentRequestSpec;
import org.chromium.components.payments.PaymentRequestUpdateEventListener;
import org.chromium.components.payments.PaymentValidator;
import org.chromium.components.payments.Section;
import org.chromium.components.payments.UrlUtil;
import org.chromium.components.security_state.SecurityStateModel;
import org.chromium.components.url_formatter.UrlFormatter;
import org.chromium.content_public.browser.RenderFrameHost;
import org.chromium.content_public.browser.WebContents;
import org.chromium.content_public.browser.WebContentsStatics;
import org.chromium.mojo.system.MojoException;
import org.chromium.payments.mojom.AddressErrors;
import org.chromium.payments.mojom.CanMakePaymentQueryResult;
import org.chromium.payments.mojom.HasEnrolledInstrumentQueryResult;
import org.chromium.payments.mojom.PayerDetail;
import org.chromium.payments.mojom.PayerErrors;
import org.chromium.payments.mojom.PaymentAddress;
import org.chromium.payments.mojom.PaymentComplete;
import org.chromium.payments.mojom.PaymentCurrencyAmount;
import org.chromium.payments.mojom.PaymentDetails;
import org.chromium.payments.mojom.PaymentDetailsModifier;
import org.chromium.payments.mojom.PaymentErrorReason;
import org.chromium.payments.mojom.PaymentItem;
import org.chromium.payments.mojom.PaymentMethodData;
import org.chromium.payments.mojom.PaymentOptions;
import org.chromium.payments.mojom.PaymentRequest;
import org.chromium.payments.mojom.PaymentRequestClient;
import org.chromium.payments.mojom.PaymentResponse;
import org.chromium.payments.mojom.PaymentShippingOption;
import org.chromium.payments.mojom.PaymentShippingType;
import org.chromium.payments.mojom.PaymentValidationErrors;
import org.chromium.url.GURL;
import org.chromium.url.Origin;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.Comparator;
import java.util.HashMap;
import java.util.HashSet;
import java.util.LinkedList;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.Queue;
import java.util.Set;

/**
 * This is the Clank specific parts of {@link PaymentRequest}, with the parts shared with WebLayer
 * living in {@link ComponentPaymentRequestImpl}.
 */
public class PaymentRequestImpl
        implements ComponentPaymentRequestDelegate, PaymentRequestUI.Client,
                   PaymentAppFactoryDelegate, PaymentAppFactoryParams,
                   PaymentRequestUpdateEventListener, PaymentApp.AbortCallback,
                   PaymentApp.InstrumentDetailsCallback,
                   PaymentResponseHelper.PaymentResponseRequesterDelegate, FocusChangedObserver,
                   NormalizedAddressRequestDelegate, PaymentDetailsConverter.MethodChecker,
                   PaymentHandlerUiObserver, PaymentUIsManager.Delegate {
    /**
     * A delegate to ask questions about the system, that allows tests to inject behaviour without
     * having to modify the entire system. This partially mirrors a similar C++
     * (Content)PaymentRequestDelegate for the C++ implementation, allowing the test harness to
     * override behaviour in both in a similar fashion.
     */
    public interface Delegate {
        /**
         * Returns whether the ChromeActivity is currently showing an OffTheRecord tab.
         */
        boolean isOffTheRecord(ChromeActivity activity);
        /**
         * Returns a non-null string if there is an invalid SSL certificate on the currently
         * loaded page.
         */
        String getInvalidSslCertificateErrorMessage();
        /**
         * Returns true if the web contents that initiated the payment request is active.
         */
        boolean isWebContentsActive(@NonNull ChromeActivity activity);
        /**
         * Returns whether the preferences allow CAN_MAKE_PAYMENT.
         */
        boolean prefsCanMakePayment();
        /**
         * Returns true if the UI can be skipped for "basic-card" scenarios. This will only ever
         * be true in tests.
         */
        boolean skipUiForBasicCard();
        /**
         * If running inside of a Trusted Web Activity, returns the package name for Trusted Web
         * Activity. Otherwise returns an empty string or null.
         */
        @Nullable
        String getTwaPackageName(@Nullable ChromeActivity activity);
    }

    /**
     * A test-only observer for the PaymentRequest service implementation.
     */
    public interface PaymentRequestServiceObserverForTest {
        /**
         * Called after an instance of PaymentRequestImpl has been created.
         *
         * @param paymentRequest The newly created instance of PaymentRequestImpl.
         */
        void onPaymentRequestCreated(PaymentRequestImpl paymentRequest);

        /**
         * Called when an abort request was denied.
         */
        void onPaymentRequestServiceUnableToAbort();

        /**
         * Called when the controller is notified of billing address change, but does not alter the
         * editor UI.
         */
        void onPaymentRequestServiceBillingAddressChangeProcessed();

        /**
         * Called when the controller is notified of an expiration month change.
         */
        void onPaymentRequestServiceExpirationMonthChange();

        /**
         * Called when a show request failed. This can happen when:
         * <ul>
         *   <li>The merchant requests only unsupported payment methods.</li>
         *   <li>The merchant requests only payment methods that don't have corresponding apps and
         *   are not able to add a credit card from PaymentRequest UI.</li>
         * </ul>
         */
        void onPaymentRequestServiceShowFailed();

        /**
         * Called when the canMakePayment() request has been responded to.
         */
        void onPaymentRequestServiceCanMakePaymentQueryResponded();

        /**
         * Called when the hasEnrolledInstrument() request has been responded to.
         */
        void onPaymentRequestServiceHasEnrolledInstrumentQueryResponded();

        /**
         * Called when the payment response is ready.
         */
        void onPaymentResponseReady();

        /**
         * Called when the browser acknowledges the renderer's complete call, which indicates that
         * the browser UI has closed.
         */
        void onCompleteReplied();

        /**
         * Called when the renderer is closing the mojo connection (e.g. upon show promise
         * rejection).
         */
        void onRendererClosedMojoConnection();
    }

    /**
     * An observer interface injected when running tests to allow them to observe events.
     * This interface holds events that should be passed back to the native C++ test
     * harness and mirrors the C++ PaymentRequest::ObserverForTest() interface. Its methods
     * should be called in the same places that the C++ PaymentRequest object will call its
     * ObserverForTest.
     */
    public interface NativeObserverForTest {
        void onCanMakePaymentCalled();
        void onCanMakePaymentReturned();
        void onHasEnrolledInstrumentCalled();
        void onHasEnrolledInstrumentReturned();
        void onAppListReady(@Nullable List<EditableOption> paymentApps, PaymentItem total);
        void onNotSupportedError();
        void onConnectionTerminated();
        void onAbortCalled();
        void onCompleteCalled();
        void onMinimalUIReady();
    }

    /** Limit in the number of suggested items in a section. */
    public static final int SUGGESTIONS_LIMIT = 4;

    private static final String TAG = "PaymentRequest";
    // Reverse order of the comparator to sort in descending order of completeness scores.
    private static final Comparator<Completable> COMPLETENESS_COMPARATOR =
            (a, b) -> (PaymentAppComparator.compareCompletablesByCompleteness(b, a));

    private ComponentPaymentRequestImpl mComponentPaymentRequestImpl;

    private PaymentOptions mPaymentOptions;
    private boolean mRequestShipping;
    private boolean mRequestPayerName;
    private boolean mRequestPayerPhone;
    private boolean mRequestPayerEmail;

    private final Comparator<PaymentApp> mPaymentAppComparator;

    private static PaymentRequestServiceObserverForTest sObserverForTest;
    private static boolean sIsLocalCanMakePaymentQueryQuotaEnforcedForTest;

    /**
     * Hold the currently showing PaymentRequest. Used to prevent showing more than one
     * PaymentRequest UI per browser process.
     */
    private static PaymentRequestImpl sShowingPaymentRequest;

    /** Monitors changes in the TabModelSelector. */
    private final TabModelSelectorObserver mSelectorObserver = new EmptyTabModelSelectorObserver() {
        @Override
        public void onTabModelSelected(TabModel newModel, TabModel oldModel) {
            mJourneyLogger.setAborted(AbortReason.ABORTED_BY_USER);
            disconnectFromClientWithDebugMessage(ErrorStrings.TAB_SWITCH);
        }
    };

    /** Monitors changes in the current TabModel. */
    private final TabModelObserver mTabModelObserver = new TabModelObserver() {
        @Override
        public void didSelectTab(Tab tab, @TabSelectionType int type, int lastId) {
            if (tab == null || tab.getId() != lastId) {
                mJourneyLogger.setAborted(AbortReason.ABORTED_BY_USER);
                disconnectFromClientWithDebugMessage(ErrorStrings.TAB_SWITCH);
            }
        }
    };

    /** Monitors changes in the tab overview. */
    private final OverviewModeObserver mOverviewModeObserver = new EmptyOverviewModeObserver() {
        @Override
        public void onOverviewModeStartedShowing(boolean showToolbar) {
            mJourneyLogger.setAborted(AbortReason.ABORTED_BY_USER);
            disconnectFromClientWithDebugMessage(ErrorStrings.TAB_OVERVIEW_MODE);
        }
    };

    private final Handler mHandler = new Handler();
    private final RenderFrameHost mRenderFrameHost;
    private final Delegate mDelegate;
    private final NativeObserverForTest mNativeObserverForTest;
    private final WebContents mWebContents;
    private final String mTopLevelOrigin;
    private final String mPaymentRequestOrigin;
    private final Origin mPaymentRequestSecurityOrigin;
    private final String mMerchantName;
    @Nullable
    private final byte[][] mCertificateChain;
    private final JourneyLogger mJourneyLogger;
    private final boolean mIsOffTheRecord;

    private List<AutofillProfile> mAutofillProfiles;
    private boolean mHaveRequestedAutofillData = true;
    private boolean mIsCanMakePaymentResponsePending;
    private boolean mIsHasEnrolledInstrumentResponsePending;
    private boolean mHasEnrolledInstrumentUsesPerMethodQuota;
    private boolean mIsCurrentPaymentRequestShowing;
    private boolean mWasRetryCalled;
    private final Queue<Runnable> mRetryQueue = new LinkedList<>();

    /**
     * The raw total amount being charged, as it was received from the website. This data is passed
     * to the payment app.
     */
    private PaymentItem mRawTotal;

    /**
     * The raw items in the shopping cart, as they were received from the website. This data is
     * passed to the payment app.
     */
    private List<PaymentItem> mRawLineItems;

    /**
     * The raw shipping options, as it was received from the website. This data is passed to the
     * payment app when the app is responsible for handling shipping address.
     */
    private List<PaymentShippingOption> mRawShippingOptions;

    /**
     * A mapping from method names to modifiers, which include modified totals and additional line
     * items. Used to display modified totals for each payment apps, modified total in order
     * summary, and additional line items in order summary.
     */
    private Map<String, PaymentDetailsModifier> mModifiers;

    /**
     * The UI model of the shopping cart, including the total. Each item includes a label and a
     * price string. This data is passed to the UI.
     */
    private ShoppingCart mUiShoppingCart;

    /**
     * The UI model for the shipping options. Includes the label and sublabel for each shipping
     * option. Also keeps track of the selected shipping option. This data is passed to the UI.
     */
    private SectionInformation mUiShippingOptions;

    private PaymentRequestSpec mSpec;
    private String mId;
    private Map<String, PaymentMethodData> mMethodData;
    private int mShippingType;
    private boolean mIsFinishedQueryingPaymentApps;
    private List<PaymentApp> mPendingApps = new ArrayList<>();
    private final PaymentUIsManager mPaymentUIsManager;
    private MinimalUICoordinator mMinimalUi;
    private Callback<PaymentInformation> mPaymentInformationCallback;
    private PaymentApp mInvokedPaymentApp;
    private PaymentHandlerCoordinator mPaymentHandlerUi;
    private boolean mHideServerAutofillCards;
    private ContactEditor mContactEditor;
    private boolean mHasRecordedAbortReason;
    private boolean mWaitForUpdatedDetails;
    private Map<String, CurrencyFormatter> mCurrencyFormatterMap;
    private TabModelSelector mObservedTabModelSelector;
    private TabModel mObservedTabModel;
    private OverviewModeBehavior mOverviewModeBehavior;
    private PaymentHandlerHost mPaymentHandlerHost;

    /**
     * True when at least one url payment method identifier is specified in payment request.
     */
    private boolean mURLPaymentMethodIdentifiersSupported;

    /**
     * A mapping of the payment method names to the corresponding payment method specific data. If
     * STRICT_HAS_ENROLLED_AUTOFILL_INSTRUMENT is enabled, then the key "basic-card-payment-options"
     * also maps to the following payment options:
     *  - requestPayerEmail
     *  - requestPayerName
     *  - requestPayerPhone
     *  - requestShipping
     */
    private Map<String, PaymentMethodData> mQueryForQuota;

    /**
     * There are a few situations were the Payment Request can appear, from a code perspective, to
     * be shown more than once. This boolean is used to make sure it is only logged once.
     */
    private boolean mDidRecordShowEvent;

    /** True if any of the requested payment methods are supported. */
    private boolean mCanMakePayment;

    /**
     * True after at least one usable payment app has been found and the setting allows querying
     * this value. This value can be used to respond to hasEnrolledInstrument(). Should be read only
     * after all payment apps have been queried.
     */
    private boolean mHasEnrolledInstrument;

    /**
     * Whether there's at least one app that is not an autofill card. Should be read only after all
     * payment apps have been queried.
     */
    private boolean mHasNonAutofillApp;

    /**
     * True if we should skip showing PaymentRequest UI.
     *
     * <p>In cases where there is a single payment app and the merchant does not request shipping
     * or billing, we can skip showing UI as Payment Request UI is not benefiting the user at all.
     */
    private boolean mShouldSkipShowingPaymentRequestUi;

    /** Whether PaymentRequest.show() was invoked with a user gesture. */
    private boolean mIsUserGestureShow;

    /** The helper to create and fill the response to send to the merchant. */
    private PaymentResponseHelper mPaymentResponseHelper;

    /** If not empty, use this error message for rejecting PaymentRequest.show(). */
    private String mRejectShowErrorMessage;

    /**
     * True when Payment Request is invoked on a prohibited origin (e.g., blob:) or with an invalid
     * SSL certificate (e.g., self-signed).
     */
    private boolean mIsProhibitedOriginOrInvalidSsl;

    /** A helper to manage the Skip-to-GPay experimental flow. */
    private SkipToGPayHelper mSkipToGPayHelper;

    /**
     * When true skip UI is available for non-url based payment method identifiers (e.g.
     * basic-card).
     */
    private boolean mSkipUiForNonUrlPaymentMethodIdentifiers;

    /**
     * Builds the PaymentRequest service implementation.
     *
     * @param renderFrameHost The host of the frame that has invoked the PaymentRequest API.
     */
    public PaymentRequestImpl(RenderFrameHost renderFrameHost, Delegate delegate,
            NativeObserverForTest nativeObserver) {
        assert renderFrameHost != null;

        mRenderFrameHost = renderFrameHost;
        mDelegate = delegate;
        mNativeObserverForTest = nativeObserver;
        mWebContents = WebContentsStatics.fromRenderFrameHost(renderFrameHost);

        mPaymentRequestOrigin =
                UrlFormatter.formatUrlForSecurityDisplay(mRenderFrameHost.getLastCommittedURL());
        mPaymentRequestSecurityOrigin = mRenderFrameHost.getLastCommittedOrigin();
        mTopLevelOrigin =
                UrlFormatter.formatUrlForSecurityDisplay(mWebContents.getLastCommittedUrl());

        mMerchantName = mWebContents.getTitle();

        mCertificateChain = CertificateChainHelper.getCertificateChain(mWebContents);

        mIsOffTheRecord = mDelegate.isOffTheRecord(ChromeActivity.fromWebContents(mWebContents));

        // Do not persist changes on disk in OffTheRecord mode.
        AddressEditor addressEditor = new AddressEditor(
                AddressEditor.Purpose.PAYMENT_REQUEST, /*saveToDisk=*/!mIsOffTheRecord);
        // PaymentRequest card editor does not show the organization name in the dropdown with the
        // billing address labels.
        CardEditor cardEditor = new CardEditor(
                mWebContents, addressEditor, /*includeOrgLabel=*/false, sObserverForTest);

        mJourneyLogger = new JourneyLogger(mIsOffTheRecord, mWebContents);
        mCurrencyFormatterMap = new HashMap<>();

        mSkipUiForNonUrlPaymentMethodIdentifiers = mDelegate.skipUiForBasicCard();

        if (sObserverForTest != null) sObserverForTest.onPaymentRequestCreated(this);
        mPaymentUIsManager = new PaymentUIsManager(/*delegate=*/this, addressEditor, cardEditor);
        mPaymentAppComparator = new PaymentAppComparator(/*params=*/this);
    }

    // Implement ComponentPaymentRequestDelegate:
    @Override
    public void setComponentPaymentRequestImpl(
            ComponentPaymentRequestImpl componentPaymentRequestImpl) {
        assert mComponentPaymentRequestImpl == null;
        assert componentPaymentRequestImpl != null;
        mComponentPaymentRequestImpl = componentPaymentRequestImpl;
    }

    // Implement ComponentPaymentRequestDelegate:
    /**
     * Called by the merchant website to initialize the payment request data.
     */
    @Override
    public void init(PaymentMethodData[] methodData, PaymentDetails details,
            @Nullable PaymentOptions options, boolean googlePayBridgeEligible) {
        assert getClient() != null;
        mMethodData = new HashMap<>();
        mComponentPaymentRequestImpl.registerPaymentRequestLifecycleObserver(mPaymentUIsManager);

        if (!OriginSecurityChecker.isOriginSecure(mWebContents.getLastCommittedUrl())) {
            mJourneyLogger.setAborted(AbortReason.INVALID_DATA_FROM_RENDERER);
            disconnectFromClientWithDebugMessage(ErrorStrings.NOT_IN_A_SECURE_ORIGIN);
            return;
        }

        mPaymentOptions = options;
        mRequestShipping = options != null && options.requestShipping;
        mRequestPayerName = options != null && options.requestPayerName;
        mRequestPayerPhone = options != null && options.requestPayerPhone;
        mRequestPayerEmail = options != null && options.requestPayerEmail;
        mShippingType = options == null ? PaymentShippingType.SHIPPING : options.shippingType;

        // TODO(crbug.com/978471): Improve architecture for handling prohibited origins and invalid
        // SSL certificates.
        if (!UrlUtil.isOriginAllowedToUseWebPaymentApis(mWebContents.getLastCommittedUrl())) {
            mIsProhibitedOriginOrInvalidSsl = true;
            mRejectShowErrorMessage = ErrorStrings.PROHIBITED_ORIGIN;
            Log.d(TAG, mRejectShowErrorMessage);
            Log.d(TAG, ErrorStrings.PROHIBITED_ORIGIN_OR_INVALID_SSL_EXPLANATION);
            // Don't show any UI. Resolve .canMakePayment() with "false". Reject .show() with
            // "NotSupportedError".
            mQueryForQuota = new HashMap<>();
            onDoneCreatingPaymentApps(/*factory=*/null);
            return;
        }

        mJourneyLogger.setRequestedInformation(
                mRequestShipping, mRequestPayerEmail, mRequestPayerPhone, mRequestPayerName);

        assert mRejectShowErrorMessage == null;
        mRejectShowErrorMessage = mDelegate.getInvalidSslCertificateErrorMessage();
        if (!TextUtils.isEmpty(mRejectShowErrorMessage)) {
            mIsProhibitedOriginOrInvalidSsl = true;
            Log.d(TAG, mRejectShowErrorMessage);
            Log.d(TAG, ErrorStrings.PROHIBITED_ORIGIN_OR_INVALID_SSL_EXPLANATION);
            // Don't show any UI. Resolve .canMakePayment() with "false". Reject .show() with
            // "NotSupportedError".
            mQueryForQuota = new HashMap<>();
            onDoneCreatingPaymentApps(/*factory=*/null);
            return;
        }

        boolean googlePayBridgeActivated = googlePayBridgeEligible
                && SkipToGPayHelper.canActivateExperiment(mWebContents, methodData);

        mMethodData = getValidatedMethodData(
                methodData, googlePayBridgeActivated, mPaymentUIsManager.getCardEditor());
        if (mMethodData == null) {
            mJourneyLogger.setAborted(AbortReason.INVALID_DATA_FROM_RENDERER);
            disconnectFromClientWithDebugMessage(ErrorStrings.INVALID_PAYMENT_METHODS_OR_DATA);
            return;
        }

        if (googlePayBridgeActivated) {
            PaymentMethodData data = mMethodData.get(MethodStrings.GOOGLE_PAY);
            mSkipToGPayHelper = new SkipToGPayHelper(options, data.gpayBridgeData);
        }

        mQueryForQuota = new HashMap<>(mMethodData);
        if (mQueryForQuota.containsKey(MethodStrings.BASIC_CARD)
                && PaymentFeatureList.isEnabledOrExperimentalFeaturesEnabled(
                        PaymentFeatureList.STRICT_HAS_ENROLLED_AUTOFILL_INSTRUMENT)) {
            PaymentMethodData paymentMethodData = new PaymentMethodData();
            paymentMethodData.stringifiedData =
                    PaymentOptionsUtils.stringifyRequestedInformation(mPaymentOptions);
            mQueryForQuota.put("basic-card-payment-options", paymentMethodData);
        }

        if (!parseAndValidateDetailsOrDisconnectFromClient(details)) return;
        mSpec = new PaymentRequestSpec(mPaymentOptions, details, mMethodData.values(),
                LocaleUtils.getDefaultLocaleString());

        if (mRawTotal == null) {
            mJourneyLogger.setAborted(AbortReason.INVALID_DATA_FROM_RENDERER);
            disconnectFromClientWithDebugMessage(ErrorStrings.TOTAL_REQUIRED);
            return;
        }
        mId = details.id;

        // The first time initializations and validation of all of the parameters of {@link
        // PaymentRequestParams} should be done before {@link
        // PaymentRequestLifeCycleObserver#onPaymentRequestParamsInitiated}.
        mComponentPaymentRequestImpl.getPaymentRequestLifecycleObserver()
                .onPaymentRequestParamsInitiated(
                        /*params=*/this);

        if (PaymentOptionsUtils.requestAnyInformation(mPaymentOptions)) {
            mAutofillProfiles = Collections.unmodifiableList(
                    PersonalDataManager.getInstance().getProfilesToSuggest(
                            false /* includeNameInLabel */));
        }

        if (mRequestShipping) {
            boolean haveCompleteShippingAddress = false;
            for (int i = 0; i < mAutofillProfiles.size(); i++) {
                if (AutofillAddress.checkAddressCompletionStatus(
                            mAutofillProfiles.get(i), AutofillAddress.CompletenessCheckType.NORMAL)
                        == AutofillAddress.CompletionStatus.COMPLETE) {
                    haveCompleteShippingAddress = true;
                    break;
                }
            }
            mHaveRequestedAutofillData &= haveCompleteShippingAddress;
        }

        if (mRequestPayerName || mRequestPayerPhone || mRequestPayerEmail) {
            // Do not persist changes on disk in OffTheRecord mode.
            mContactEditor = new ContactEditor(mRequestPayerName, mRequestPayerPhone,
                    mRequestPayerEmail, /*saveToDisk=*/!mIsOffTheRecord);
            boolean haveCompleteContactInfo = false;
            for (int i = 0; i < mAutofillProfiles.size(); i++) {
                AutofillProfile profile = mAutofillProfiles.get(i);
                if (mContactEditor.checkContactCompletionStatus(profile.getFullName(),
                            profile.getPhoneNumber(), profile.getEmailAddress())
                        == ContactEditor.COMPLETE) {
                    haveCompleteContactInfo = true;
                    break;
                }
            }
            mHaveRequestedAutofillData &= haveCompleteContactInfo;
        }

        PaymentAppService.getInstance().create(/*delegate=*/this);

        // Log the various types of payment methods that were requested by the merchant.
        boolean requestedMethodGoogle = false;
        // Not to record requestedMethodBasicCard because JourneyLogger ignore the case where the
        // specified networks are unsupported. mPaymentUIsManager.merchantSupportsAutofillCards()
        // better captures this group of interest than requestedMethodBasicCard.
        boolean requestedMethodOther = false;
        mURLPaymentMethodIdentifiersSupported = false;
        for (String methodName : mMethodData.keySet()) {
            switch (methodName) {
                case MethodStrings.ANDROID_PAY:
                case MethodStrings.GOOGLE_PAY:
                    mURLPaymentMethodIdentifiersSupported = true;
                    requestedMethodGoogle = true;
                    break;
                case MethodStrings.BASIC_CARD:
                    // Not to record requestedMethodBasicCard because
                    // mPaymentUIsManager.merchantSupportsAutofillCards() is used instead.
                    break;
                default:
                    // "Other" includes https url, http url(when certifate check is bypassed) and
                    // the unlisted methods defined in {@link MethodStrings}.
                    requestedMethodOther = true;
                    if (methodName.startsWith(UrlConstants.HTTPS_URL_PREFIX)
                            || methodName.startsWith(UrlConstants.HTTP_URL_PREFIX)) {
                        mURLPaymentMethodIdentifiersSupported = true;
                    }
            }
        }
        mJourneyLogger.setRequestedPaymentMethodTypes(
                /*requestedBasicCard=*/mPaymentUIsManager.merchantSupportsAutofillCards(),
                requestedMethodGoogle, requestedMethodOther);
    }

    /**
     * Calculate whether the browser payment sheet should be skipped directly into the payment app.
     */
    private void calculateWhetherShouldSkipShowingPaymentRequestUi() {
        // This should be called after all payment apps are ready and request.show() is called,
        // since only then whether or not should skip payment sheet UI is determined.
        assert mIsFinishedQueryingPaymentApps;
        assert mIsCurrentPaymentRequestShowing;

        assert mPaymentUIsManager.getPaymentMethodsSection() != null;
        PaymentApp selectedApp =
                (PaymentApp) mPaymentUIsManager.getPaymentMethodsSection().getSelectedItem();

        // If there is only a single payment app which can provide all merchant requested
        // information, we can safely go directly to the payment app instead of showing Payment
        // Request UI.
        mShouldSkipShowingPaymentRequestUi =
                PaymentFeatureList.isEnabled(PaymentFeatureList.WEB_PAYMENTS_SINGLE_APP_UI_SKIP)
                // Only allowing payment apps that own their own UIs.
                // This excludes AutofillPaymentInstrument as its UI is rendered inline in
                // the payment request UI, thus can't be skipped.
                && (mURLPaymentMethodIdentifiersSupported
                        || mSkipUiForNonUrlPaymentMethodIdentifiers)
                && mPaymentUIsManager.getPaymentMethodsSection().getSize() >= 1
                && onlySingleAppCanProvideAllRequiredInformation()
                // Skip to payment app only if it can be pre-selected.
                && selectedApp != null
                // Skip to payment app only if user gesture is provided when it is required to
                // skip-UI.
                && (mIsUserGestureShow || !selectedApp.isUserGestureRequiredToSkipUi());
    }

    /**
     * @return true when there is exactly one available payment app which can provide all requested
     * information including shipping address and payer's contact information whenever needed.
     */
    private boolean onlySingleAppCanProvideAllRequiredInformation() {
        assert mPaymentUIsManager.getPaymentMethodsSection() != null;

        if (!mRequestShipping && !mRequestPayerName && !mRequestPayerPhone && !mRequestPayerEmail) {
            return mPaymentUIsManager.getPaymentMethodsSection().getSize() == 1
                    && !((PaymentApp) mPaymentUIsManager.getPaymentMethodsSection().getItem(0))
                                .isAutofillInstrument();
        }

        boolean anAppCanProvideAllInfo = false;
        int sectionSize = mPaymentUIsManager.getPaymentMethodsSection().getSize();
        for (int i = 0; i < sectionSize; i++) {
            PaymentApp app = (PaymentApp) mPaymentUIsManager.getPaymentMethodsSection().getItem(i);
            if ((!mRequestShipping || app.handlesShippingAddress())
                    && (!mRequestPayerName || app.handlesPayerName())
                    && (!mRequestPayerPhone || app.handlesPayerPhone())
                    && (!mRequestPayerEmail || app.handlesPayerEmail())) {
                // There is more than one available app that can provide all merchant requested
                // information information.
                if (anAppCanProvideAllInfo) return false;

                anAppCanProvideAllInfo = true;
            }
        }
        return anAppCanProvideAllInfo;
    }

    /** @return Whether the UI was built. */
    private boolean buildUI(ChromeActivity activity) {
        // Payment methods section must be ready before building the rest of the UI. This is because
        // shipping and contact sections (when requested by merchant) are populated depending on
        // whether or not the selected payment app (if such exists) can provide the required
        // information.
        assert mPaymentUIsManager.getPaymentMethodsSection() != null;

        assert activity != null;

        // Catch any time the user switches tabs. Because the dialog is modal, a user shouldn't be
        // allowed to switch tabs, which can happen if the user receives an external Intent.
        mObservedTabModelSelector = activity.getTabModelSelector();
        mObservedTabModel = activity.getCurrentTabModel();
        mObservedTabModelSelector.addObserver(mSelectorObserver);
        mObservedTabModel.addObserver(mTabModelObserver);

        // Only the currently selected tab is allowed to show the payment UI.
        if (!mDelegate.isWebContentsActive(activity)) {
            mJourneyLogger.setNotShown(NotShownReason.OTHER);
            disconnectFromClientWithDebugMessage(ErrorStrings.CANNOT_SHOW_IN_BACKGROUND_TAB);
            if (sObserverForTest != null) sObserverForTest.onPaymentRequestServiceShowFailed();
            return false;
        }

        // Catch any time the user enters the overview mode and dismiss the payment UI.
        if (activity instanceof ChromeTabbedActivity) {
            mOverviewModeBehavior = ((ChromeTabbedActivity) activity).getOverviewModeBehavior();
            if (mOverviewModeBehavior.overviewVisible()) {
                mJourneyLogger.setNotShown(NotShownReason.OTHER);
                disconnectFromClientWithDebugMessage(ErrorStrings.TAB_OVERVIEW_MODE);
                if (sObserverForTest != null) sObserverForTest.onPaymentRequestServiceShowFailed();
                return false;
            }
            mOverviewModeBehavior.addOverviewModeObserver(mOverviewModeObserver);
        }

        if (shouldShowShippingSection() && !mWaitForUpdatedDetails) {
            createShippingSection(activity, mAutofillProfiles);
        }

        if (shouldShowContactSection()) {
            mPaymentUIsManager.setContactSection(new ContactDetailsSection(
                    activity, mAutofillProfiles, mContactEditor, mJourneyLogger));
        }

        mPaymentUIsManager.setPaymentRequestUI(new PaymentRequestUI(activity, this,
                mPaymentUIsManager.merchantSupportsAutofillCards(),
                !PaymentPreferencesUtil.isPaymentCompleteOnce(), mMerchantName, mTopLevelOrigin,
                SecurityStateModel.getSecurityLevelForWebContents(mWebContents),
                new ShippingStrings(mShippingType),
                mPaymentUIsManager.getPaymentUisShowStateReconciler(),
                Profile.fromWebContents(mWebContents)));
        activity.getLifecycleDispatcher().register(
                mPaymentUIsManager
                        .getPaymentRequestUI()); // registered as a PauseResumeWithNativeObserver

        final FaviconHelper faviconHelper = new FaviconHelper();
        faviconHelper.getLocalFaviconImageForURL(Profile.fromWebContents(mWebContents),
                mWebContents.getLastCommittedUrl(),
                activity.getResources().getDimensionPixelSize(R.dimen.payments_favicon_size),
                (bitmap, iconUrl) -> {
                    PaymentRequestClient client = getClient();
                    if (client != null && bitmap == null) {
                        client.warnNoFavicon();
                    }
                    if (mPaymentUIsManager.getPaymentRequestUI() != null && bitmap != null) {
                        mPaymentUIsManager.getPaymentRequestUI().setTitleBitmap(bitmap);
                    }
                    faviconHelper.destroy();
                });

        // Add the callback to change the label of shipping addresses depending on the focus.
        if (mRequestShipping) {
            mPaymentUIsManager.getPaymentRequestUI().setShippingAddressSectionFocusChangedObserver(
                    this);
        }

        mPaymentUIsManager.getAddressEditor().setEditorDialog(
                mPaymentUIsManager.getPaymentRequestUI().getEditorDialog());
        mPaymentUIsManager.getCardEditor().setEditorDialog(
                mPaymentUIsManager.getPaymentRequestUI().getCardEditorDialog());
        if (mContactEditor != null) {
            mContactEditor.setEditorDialog(
                    mPaymentUIsManager.getPaymentRequestUI().getEditorDialog());
        }

        return true;
    }

    private void createShippingSection(
            Context context, List<AutofillProfile> unmodifiableProfiles) {
        List<AutofillAddress> addresses = new ArrayList<>();

        for (int i = 0; i < unmodifiableProfiles.size(); i++) {
            AutofillProfile profile = unmodifiableProfiles.get(i);
            mPaymentUIsManager.getAddressEditor().addPhoneNumberIfValid(profile.getPhoneNumber());

            // Only suggest addresses that have a street address.
            if (!TextUtils.isEmpty(profile.getStreetAddress())) {
                addresses.add(new AutofillAddress(context, profile));
            }
        }

        // Suggest complete addresses first.
        Collections.sort(addresses, COMPLETENESS_COMPARATOR);

        // Limit the number of suggestions.
        addresses = addresses.subList(0, Math.min(addresses.size(), SUGGESTIONS_LIMIT));

        // Load the validation rules for each unique region code.
        Set<String> uniqueCountryCodes = new HashSet<>();
        for (int i = 0; i < addresses.size(); ++i) {
            String countryCode = AutofillAddress.getCountryCode(addresses.get(i).getProfile());
            if (!uniqueCountryCodes.contains(countryCode)) {
                uniqueCountryCodes.add(countryCode);
                PersonalDataManager.getInstance().loadRulesForAddressNormalization(countryCode);
            }
        }

        // Automatically select the first address if one is complete and if the merchant does
        // not require a shipping address to calculate shipping costs.
        boolean hasCompleteShippingAddress = !addresses.isEmpty() && addresses.get(0).isComplete();
        int firstCompleteAddressIndex = SectionInformation.NO_SELECTION;
        if (mUiShippingOptions.getSelectedItem() != null && hasCompleteShippingAddress) {
            firstCompleteAddressIndex = 0;

            // The initial label for the selected shipping address should not include the
            // country.
            addresses.get(firstCompleteAddressIndex).setShippingAddressLabelWithoutCountry();
        }

        // Log the number of suggested shipping addresses and whether at least one of them is
        // complete.
        mJourneyLogger.setNumberOfSuggestionsShown(
                Section.SHIPPING_ADDRESS, addresses.size(), hasCompleteShippingAddress);

        int missingFields = 0;
        if (addresses.isEmpty()) {
            // All fields are missing.
            missingFields = AutofillAddress.CompletionStatus.INVALID_RECIPIENT
                    | AutofillAddress.CompletionStatus.INVALID_PHONE_NUMBER
                    | AutofillAddress.CompletionStatus.INVALID_ADDRESS;
        } else {
            missingFields = addresses.get(0).getMissingFieldsOfShippingProfile();
        }
        if (missingFields != 0) {
            RecordHistogram.recordSparseHistogram(
                    "PaymentRequest.MissingShippingFields", missingFields);
        }

        mPaymentUIsManager.setShippingAddressesSection(
                new SectionInformation(PaymentRequestUI.DataType.SHIPPING_ADDRESSES,
                        firstCompleteAddressIndex, addresses));
    }

    // Implement ComponentPaymentRequestDelegate:
    /**
     * Called by the merchant website to show the payment request to the user.
     */
    @Override
    public void show(boolean isUserGesture, boolean waitForUpdatedDetails) {
        if (getClient() == null) return;

        if (mPaymentUIsManager.getPaymentRequestUI() != null || mMinimalUi != null) {
            // Can be triggered only by a compromised renderer. In normal operation, calling show()
            // twice on the same instance of PaymentRequest in JavaScript is rejected at the
            // renderer level.
            mJourneyLogger.setAborted(AbortReason.INVALID_DATA_FROM_RENDERER);
            disconnectFromClientWithDebugMessage(ErrorStrings.CANNOT_SHOW_TWICE);
            return;
        }

        if (getIsAnyPaymentRequestShowing()) {
            // The renderer can create multiple instances of PaymentRequest and call show() on each
            // one. Only the first one will be shown. This also prevents multiple tabs and windows
            // from showing PaymentRequest UI at the same time.
            mJourneyLogger.setNotShown(NotShownReason.CONCURRENT_REQUESTS);
            disconnectFromClientWithDebugMessage(
                    ErrorStrings.ANOTHER_UI_SHOWING, PaymentErrorReason.ALREADY_SHOWING);
            if (sObserverForTest != null) sObserverForTest.onPaymentRequestServiceShowFailed();
            return;
        }

        setShowingPaymentRequest(this);
        mIsCurrentPaymentRequestShowing = true;
        mIsUserGestureShow = isUserGesture;
        mWaitForUpdatedDetails = waitForUpdatedDetails;

        mJourneyLogger.setTriggerTime();
        if (disconnectIfNoPaymentMethodsSupported()) return;

        ChromeActivity chromeActivity = ChromeActivity.fromWebContents(mWebContents);
        if (chromeActivity == null) {
            mJourneyLogger.setNotShown(NotShownReason.OTHER);
            disconnectFromClientWithDebugMessage(ErrorStrings.ACTIVITY_NOT_FOUND);
            if (sObserverForTest != null) sObserverForTest.onPaymentRequestServiceShowFailed();
            return;
        }

        if (mIsFinishedQueryingPaymentApps) {
            // Send AppListReady signal when all apps are created and request.show() is called.
            if (mNativeObserverForTest != null) {
                mNativeObserverForTest.onAppListReady(
                        mPaymentUIsManager.getPaymentMethodsSection().getItems(), mRawTotal);
            }
            // Calculate skip ui and build ui only after all payment apps are ready and
            // request.show() is called.
            calculateWhetherShouldSkipShowingPaymentRequestUi();
            if (!buildUI(chromeActivity)) return;
            if (!mShouldSkipShowingPaymentRequestUi && mSkipToGPayHelper == null) {
                mPaymentUIsManager.getPaymentRequestUI().show();
            }
        }

        triggerPaymentAppUiSkipIfApplicable(chromeActivity);
    }

    private void dimBackgroundIfNotBottomSheetPaymentHandler(PaymentApp selectedApp) {
        // Putting isEnabled() last is intentional. It's to ensure not to confused the unexecuted
        // group and the disabled in A/B testing.
        if (selectedApp != null
                && selectedApp.getPaymentAppType() == PaymentAppType.SERVICE_WORKER_APP
                && PaymentHandlerCoordinator.isEnabled()) {
            // When the Payment Handler (PH) UI is based on Activity, dimming the Payment
            // Request (PR) UI does not dim the PH; when it's based on bottom-sheet, dimming
            // the PR dims both UIs. As bottom-sheet itself has dimming effect, dimming PR
            // is unnecessary for the bottom-sheet PH. For now, service worker based payment apps
            // are the only ones that can open the bottom-sheet.
            return;
        }
        mPaymentUIsManager.getPaymentRequestUI().dimBackground();
    }

    private void triggerPaymentAppUiSkipIfApplicable(ChromeActivity chromeActivity) {
        // If we are skipping showing the Payment Request UI, we should call into the payment app
        // immediately after we determine the apps are ready and UI is shown.
        if ((mShouldSkipShowingPaymentRequestUi || mSkipToGPayHelper != null)
                && mIsFinishedQueryingPaymentApps && mIsCurrentPaymentRequestShowing
                && !mWaitForUpdatedDetails) {
            assert !mPaymentUIsManager.getPaymentMethodsSection().isEmpty();
            assert mPaymentUIsManager.getPaymentRequestUI() != null;

            if (isMinimalUiApplicable()) {
                triggerMinimalUi(chromeActivity);
                return;
            }

            assert !mPaymentUIsManager.getPaymentMethodsSection().isEmpty();
            PaymentApp selectedApp =
                    (PaymentApp) mPaymentUIsManager.getPaymentMethodsSection().getSelectedItem();
            dimBackgroundIfNotBottomSheetPaymentHandler(selectedApp);
            mDidRecordShowEvent = true;
            mJourneyLogger.setEventOccurred(Event.SKIPPED_SHOW);
            assert mRawTotal != null;
            // The total amount in details should be finalized at this point. So it is safe to
            // record the triggered transaction amount.
            assert !mWaitForUpdatedDetails;
            mJourneyLogger.recordTransactionAmount(
                    mRawTotal.amount.currency, mRawTotal.amount.value, false /*completed*/);
            onPayClicked(null /* selectedShippingAddress */, null /* selectedShippingOption */,
                    selectedApp);
        }
    }

    /** @return Whether the minimal UI should be shown. */
    private boolean isMinimalUiApplicable() {
        if (!mIsUserGestureShow || mPaymentUIsManager.getPaymentMethodsSection() == null
                || mPaymentUIsManager.getPaymentMethodsSection().getSize() != 1) {
            return false;
        }

        PaymentApp app =
                (PaymentApp) mPaymentUIsManager.getPaymentMethodsSection().getSelectedItem();
        if (app == null || !app.isReadyForMinimalUI() || TextUtils.isEmpty(app.accountBalance())) {
            return false;
        }

        return PaymentFeatureList.isEnabled(PaymentFeatureList.WEB_PAYMENTS_MINIMAL_UI);
    }

    /**
     * Triggers the minimal UI.
     * @param chromeActivity The Android activity for the Chrome UI that will host the minimal UI.
     */
    private void triggerMinimalUi(ChromeActivity chromeActivity) {
        // Do not show the Payment Request UI dialog even if the minimal UI is suppressed.
        mPaymentUIsManager.getPaymentUisShowStateReconciler().onBottomSheetShown();

        mMinimalUi = new MinimalUICoordinator();
        if (mMinimalUi.show(chromeActivity,
                    BottomSheetControllerProvider.from(chromeActivity.getWindowAndroid()),
                    (PaymentApp) mPaymentUIsManager.getPaymentMethodsSection().getSelectedItem(),
                    mCurrencyFormatterMap.get(mRawTotal.amount.currency),
                    mUiShoppingCart.getTotal(), this::onMinimalUIReady, this::onMinimalUiConfirmed,
                    this::onMinimalUiDismissed)) {
            mDidRecordShowEvent = true;
            mJourneyLogger.setEventOccurred(Event.SHOWN);
            return;
        }

        disconnectFromClientWithDebugMessage(ErrorStrings.MINIMAL_UI_SUPPRESSED);
    }

    private void onMinimalUIReady() {
        if (mNativeObserverForTest != null) mNativeObserverForTest.onMinimalUIReady();
    }

    private void onMinimalUiConfirmed(PaymentApp app) {
        mJourneyLogger.recordTransactionAmount(
                mRawTotal.amount.currency, mRawTotal.amount.value, false /*completed*/);
        app.disableShowingOwnUI();
        onPayClicked(null /* selectedShippingAddress */, null /* selectedShippingOption */, app);
    }

    private void onMinimalUiDismissed() {
        onDismiss();
    }

    private void onMinimalUiErroredAndClosed() {
        if (getClient() == null) return;
        closeClient();
        closeUIAndDestroyNativeObjects();
    }

    private void onMinimalUiCompletedAndClosed() {
        PaymentRequestClient client = getClient();
        if (client != null) {
            client.onComplete();
        }
        closeClient();
        closeUIAndDestroyNativeObjects();
    }

    private static Map<String, PaymentMethodData> getValidatedMethodData(
            PaymentMethodData[] methodData, boolean googlePayBridgeEligible,
            CardEditor paymentMethodsCollector) {
        // Payment methodData are required.
        if (methodData == null || methodData.length == 0) return null;
        Map<String, PaymentMethodData> result = new ArrayMap<>();
        for (int i = 0; i < methodData.length; i++) {
            String method = methodData[i].supportedMethod;

            if (TextUtils.isEmpty(method)) return null;

            if (googlePayBridgeEligible) {
                // If skip-to-GPay flow is activated, ignore all other payment methods, which can be
                // either "basic-card" or "https://android.com/pay". The latter is safe to ignore
                // because merchant has already requested Google Pay.
                if (!method.equals(MethodStrings.GOOGLE_PAY)) continue;
                if (methodData[i].gpayBridgeData != null
                        && !methodData[i].gpayBridgeData.stringifiedData.isEmpty()) {
                    methodData[i].stringifiedData = methodData[i].gpayBridgeData.stringifiedData;
                }
            }
            result.put(method, methodData[i]);

            paymentMethodsCollector.addAcceptedPaymentMethodIfRecognized(methodData[i]);
        }

        return Collections.unmodifiableMap(result);
    }

    /** Called by the payment app to get updated total based on the billing address, for example. */
    @Override
    public boolean changePaymentMethodFromInvokedApp(String methodName, String stringifiedDetails) {
        PaymentRequestClient client = getClient();
        if (TextUtils.isEmpty(methodName) || stringifiedDetails == null || client == null
                || mInvokedPaymentApp == null
                || mInvokedPaymentApp.isWaitingForPaymentDetailsUpdate()) {
            return false;
        }

        client.onPaymentMethodChange(methodName, stringifiedDetails);
        return true;
    }

    /**
     * Called by the payment app to get updated payment details based on the shipping option.
     */
    @Override
    public boolean changeShippingOptionFromInvokedApp(String shippingOptionId) {
        PaymentRequestClient client = getClient();
        if (TextUtils.isEmpty(shippingOptionId) || client == null || mInvokedPaymentApp == null
                || mInvokedPaymentApp.isWaitingForPaymentDetailsUpdate() || !mRequestShipping
                || mRawShippingOptions == null) {
            return false;
        }

        boolean isValidId = false;
        for (PaymentShippingOption option : mRawShippingOptions) {
            if (shippingOptionId.equals(option.id)) {
                isValidId = true;
                break;
            }
        }

        if (!isValidId) return false;

        client.onShippingOptionChange(shippingOptionId);
        return true;
    }

    /**
     * Called by payment app to get updated payment details based on the shipping address.
     */
    @Override
    public boolean changeShippingAddressFromInvokedApp(PaymentAddress shippingAddress) {
        PaymentRequestClient client = getClient();
        if (shippingAddress == null || client == null || mInvokedPaymentApp == null
                || mInvokedPaymentApp.isWaitingForPaymentDetailsUpdate() || !mRequestShipping) {
            return false;
        }

        redactShippingAddress(shippingAddress);
        client.onShippingAddressChange(shippingAddress);
        return true;
    }

    /**
     * Get the WebContents of the Expandable Payment Handler for testing purpose; return null if
     * nonexistent.
     *
     * @return The WebContents of the Expandable Payment Handler.
     */
    @VisibleForTesting(otherwise = VisibleForTesting.NONE)
    public static WebContents getPaymentHandlerWebContentsForTest() {
        if (sShowingPaymentRequest == null) return null;
        return sShowingPaymentRequest.getPaymentHandlerWebContentsForTestInternal();
    }

    @VisibleForTesting(otherwise = VisibleForTesting.NONE)
    private WebContents getPaymentHandlerWebContentsForTestInternal() {
        if (mPaymentHandlerUi == null) return null;
        return mPaymentHandlerUi.getWebContentsForTest();
    }

    /**
     * Click the security icon of the Expandable Payment Handler for testing purpose; return false
     * if failed.
     *
     * @return The WebContents of the Expandable Payment Handler.
     */
    @VisibleForTesting(otherwise = VisibleForTesting.NONE)
    public static boolean clickPaymentHandlerSecurityIconForTest() {
        if (sShowingPaymentRequest == null) return false;
        return sShowingPaymentRequest.clickPaymentHandlerSecurityIconForTestInternal();
    }

    @VisibleForTesting(otherwise = VisibleForTesting.NONE)
    private boolean clickPaymentHandlerSecurityIconForTestInternal() {
        if (mPaymentHandlerUi == null) return false;
        mPaymentHandlerUi.clickSecurityIconForTest();
        return true;
    }

    /**
     * Confirms payment in minimal UI. Used only in test.
     *
     * @return Whether the payment was confirmed successfully.
     */
    @VisibleForTesting(otherwise = VisibleForTesting.NONE)
    public static boolean confirmMinimalUIForTest() {
        return sShowingPaymentRequest != null
                && sShowingPaymentRequest.confirmMinimalUIForTestInternal();
    }

    @VisibleForTesting(otherwise = VisibleForTesting.NONE)
    private boolean confirmMinimalUIForTestInternal() {
        if (mMinimalUi == null) return false;
        mMinimalUi.confirmForTest();
        return true;
    }

    /**
     * Dismisses the minimal UI. Used only in test.
     *
     * @return Whether the dismissal was successful.
     */
    @VisibleForTesting(otherwise = VisibleForTesting.NONE)
    public static boolean dismissMinimalUIForTest() {
        return sShowingPaymentRequest != null
                && sShowingPaymentRequest.dismissMinimalUIForTestInternal();
    }

    @VisibleForTesting(otherwise = VisibleForTesting.NONE)
    private boolean dismissMinimalUIForTestInternal() {
        if (mMinimalUi == null) return false;
        mMinimalUi.dismissForTest();
        return true;
    }

    /**
     * Called to open a new PaymentHandler UI on the showing PaymentRequest.
     * @param url The url of the payment app to be displayed in the UI.
     * @param paymentHandlerWebContentsObserver The observer of the WebContents of the
     * PaymentHandler.
     * @return Whether the opening is successful.
     */
    public static boolean openPaymentHandlerWindow(
            GURL url, PaymentHandlerWebContentsObserver paymentHandlerWebContentsObserver) {
        return sShowingPaymentRequest != null
                && sShowingPaymentRequest.openPaymentHandlerWindowInternal(
                        url, paymentHandlerWebContentsObserver);
    }

    /**
     * Called to open a new PaymentHandler UI on this PaymentRequest.
     * @param url The url of the payment app to be displayed in the UI.
     * @param paymentHandlerWebContentsObserver The observer of the WebContents of the
     * PaymentHandler.
     * @return Whether the opening is successful.
     */
    private boolean openPaymentHandlerWindowInternal(
            GURL url, PaymentHandlerWebContentsObserver paymentHandlerWebContentsObserver) {
        assert mInvokedPaymentApp != null;
        assert mInvokedPaymentApp.getPaymentAppType() == PaymentAppType.SERVICE_WORKER_APP;

        if (mPaymentHandlerUi != null) return false;
        mPaymentHandlerUi = new PaymentHandlerCoordinator();
        ChromeActivity chromeActivity = ChromeActivity.fromWebContents(mWebContents);
        if (chromeActivity == null) return false;

        boolean success = mPaymentHandlerUi.show(chromeActivity, url, mIsOffTheRecord,
                paymentHandlerWebContentsObserver, /*uiObserver=*/this);
        if (success) {
            // UKM for payment app origin should get recorded only when the origin of the invoked
            // payment app is shown to the user.
            mJourneyLogger.setPaymentAppUkmSourceId(mInvokedPaymentApp.getUkmSourceId());
        }
        return success;
    }

    @Override
    public void onPaymentHandlerUiClosed() {
        mPaymentUIsManager.getPaymentUisShowStateReconciler().onBottomSheetClosed();
        mPaymentHandlerUi = null;
    }

    @Override
    public void onPaymentHandlerUiShown() {
        assert mPaymentHandlerUi != null;
        mPaymentUIsManager.getPaymentUisShowStateReconciler().onBottomSheetShown();
    }

    @Override
    public boolean isInvokedInstrumentValidForPaymentMethodIdentifier(String methodName) {
        return mInvokedPaymentApp != null
                && mInvokedPaymentApp.isValidForPaymentMethodData(methodName, null);
    }

    // Implement ComponentPaymentRequestDelegate:
    /**
     * Called by merchant to update the shipping options and line items after the user has selected
     * their shipping address or shipping option.
     */
    @Override
    public void updateWith(PaymentDetails details) {
        if (getClient() == null) return;

        if (mWaitForUpdatedDetails) {
            initializeWithUpdatedDetails(details);
            return;
        }

        if (mPaymentUIsManager.getPaymentRequestUI() == null) {
            mJourneyLogger.setAborted(AbortReason.INVALID_DATA_FROM_RENDERER);
            disconnectFromClientWithDebugMessage(ErrorStrings.CANNOT_UPDATE_WITHOUT_SHOW);
            return;
        }

        if (!PaymentOptionsUtils.requestAnyInformation(mPaymentOptions)
                && (mInvokedPaymentApp == null
                        || !mInvokedPaymentApp.isWaitingForPaymentDetailsUpdate())) {
            mJourneyLogger.setAborted(AbortReason.INVALID_DATA_FROM_RENDERER);
            disconnectFromClientWithDebugMessage(ErrorStrings.INVALID_STATE);
            return;
        }

        if (!parseAndValidateDetailsOrDisconnectFromClient(details)) return;
        mSpec.updateWith(details);

        if (mInvokedPaymentApp != null && mInvokedPaymentApp.isWaitingForPaymentDetailsUpdate()) {
            // After a payment app has been invoked, all of the merchant's calls to update the price
            // via updateWith() should be forwarded to the invoked app, so it can reflect the
            // updated price in its UI.
            mInvokedPaymentApp.updateWith(
                    PaymentDetailsConverter.convertToPaymentRequestDetailsUpdate(details,
                            mInvokedPaymentApp.handlesShippingAddress() /* handlesShipping */,
                            this /* methodChecker */));
            return;
        }

        if (shouldShowShippingSection()
                && (mUiShippingOptions.isEmpty() || !TextUtils.isEmpty(details.error))
                && mPaymentUIsManager.getShippingAddressesSection().getSelectedItem() != null) {
            mPaymentUIsManager.getShippingAddressesSection().getSelectedItem().setInvalid();
            mPaymentUIsManager.getShippingAddressesSection().setSelectedItemIndex(
                    SectionInformation.INVALID_SELECTION);
            mPaymentUIsManager.getShippingAddressesSection().setErrorMessage(details.error);
        }

        enableUserInterfaceAfterPaymentRequestUpdateEvent();
    }

    private void initializeWithUpdatedDetails(PaymentDetails details) {
        assert mWaitForUpdatedDetails;

        ChromeActivity chromeActivity = ChromeActivity.fromWebContents(mWebContents);
        if (chromeActivity == null) {
            mJourneyLogger.setNotShown(NotShownReason.OTHER);
            disconnectFromClientWithDebugMessage(ErrorStrings.ACTIVITY_NOT_FOUND);
            return;
        }

        if (!parseAndValidateDetailsOrDisconnectFromClient(details)) return;
        mSpec.updateWith(details);

        if (!TextUtils.isEmpty(details.error)) {
            mJourneyLogger.setNotShown(NotShownReason.OTHER);
            disconnectFromClientWithDebugMessage(ErrorStrings.INVALID_STATE);
            return;
        }

        // Do not create shipping section When UI is not built yet. This happens when the show
        // promise gets resolved before all apps are ready.
        if (mPaymentUIsManager.getPaymentRequestUI() != null && shouldShowShippingSection()) {
            createShippingSection(chromeActivity, mAutofillProfiles);
        }

        mWaitForUpdatedDetails = false;
        // Triggered tansaction amount gets recorded when both of the following conditions are met:
        // 1- Either Event.Shown or Event.SKIPPED_SHOW bits are set showing that transaction is
        // triggered (mDidRecordShowEvent == true). 2- The total amount in details won't change
        // (mWaitForUpdatedDetails == false). Record the transaction amount only when the triggered
        // condition is already met. Otherwise it will get recorded when triggered condition becomes
        // true.
        if (mDidRecordShowEvent) {
            assert mRawTotal != null;
            mJourneyLogger.recordTransactionAmount(
                    mRawTotal.amount.currency, mRawTotal.amount.value, false /*completed*/);
        }

        triggerPaymentAppUiSkipIfApplicable(chromeActivity);

        if (mIsFinishedQueryingPaymentApps && !mShouldSkipShowingPaymentRequestUi) {
            enableUserInterfaceAfterPaymentRequestUpdateEvent();
        }
    }

    // Implement ComponentPaymentRequestDelegate:
    /**
     * Called when the merchant received a new shipping address, shipping option, or payment method
     * info, but did not update the payment details in response.
     */
    @Override
    public void onPaymentDetailsNotUpdated() {
        if (getClient() == null) return;

        if (mPaymentUIsManager.getPaymentRequestUI() == null) {
            mJourneyLogger.setAborted(AbortReason.INVALID_DATA_FROM_RENDERER);
            disconnectFromClientWithDebugMessage(ErrorStrings.CANNOT_UPDATE_WITHOUT_SHOW);
            return;
        }

        if (mInvokedPaymentApp != null && mInvokedPaymentApp.isWaitingForPaymentDetailsUpdate()) {
            mInvokedPaymentApp.onPaymentDetailsNotUpdated();
            return;
        }

        enableUserInterfaceAfterPaymentRequestUpdateEvent();
    }

    private void enableUserInterfaceAfterPaymentRequestUpdateEvent() {
        if (mPaymentInformationCallback != null
                && mPaymentUIsManager.getPaymentMethodsSection() != null) {
            providePaymentInformation();
        } else {
            mPaymentUIsManager.getPaymentRequestUI().updateOrderSummarySection(mUiShoppingCart);
            if (shouldShowShippingSection()) {
                mPaymentUIsManager.getPaymentRequestUI().updateSection(
                        PaymentRequestUI.DataType.SHIPPING_OPTIONS, mUiShippingOptions);
            }
        }
    }

    /**
     * Sets the total, display line items, and shipping options based on input and returns the
     * status boolean. That status is true for valid data, false for invalid data. If the input is
     * invalid, disconnects from the client. Both raw and UI versions of data are updated.
     *
     * @param details The total, line items, and shipping options to parse, validate, and save in
     *                member variables.
     * @return True if the data is valid. False if the data is invalid.
     */
    private boolean parseAndValidateDetailsOrDisconnectFromClient(PaymentDetails details) {
        if (!PaymentValidator.validatePaymentDetails(details)) {
            mJourneyLogger.setAborted(AbortReason.INVALID_DATA_FROM_RENDERER);
            disconnectFromClientWithDebugMessage(ErrorStrings.INVALID_PAYMENT_DETAILS);
            return false;
        }

        if (details.total != null) {
            mRawTotal = details.total;
        }

        if (mRawLineItems == null || details.displayItems != null) {
            mRawLineItems = Collections.unmodifiableList(details.displayItems != null
                            ? Arrays.asList(details.displayItems)
                            : new ArrayList<>());
        }

        loadCurrencyFormattersForPaymentDetails(details);

        // Total is never pending.
        CurrencyFormatter formatter = getOrCreateCurrencyFormatter(mRawTotal.amount);
        LineItem uiTotal = new LineItem(mRawTotal.label, formatter.getFormattedCurrencyCode(),
                formatter.format(mRawTotal.amount.value), /* isPending */ false);

        List<LineItem> uiLineItems = getLineItems(mRawLineItems);

        mUiShoppingCart = new ShoppingCart(uiTotal, uiLineItems);

        if (mUiShippingOptions == null || details.shippingOptions != null) {
            mUiShippingOptions = getShippingOptions(details.shippingOptions);
        }

        if (mSkipToGPayHelper != null && !mSkipToGPayHelper.setShippingOption(details)) {
            return false;
        }

        if (details.modifiers != null) {
            if (details.modifiers.length == 0 && mModifiers != null) mModifiers.clear();

            for (int i = 0; i < details.modifiers.length; i++) {
                PaymentDetailsModifier modifier = details.modifiers[i];
                String method = modifier.methodData.supportedMethod;
                if (mModifiers == null) mModifiers = new ArrayMap<>();
                mModifiers.put(method, modifier);
            }
        }

        if (details.shippingOptions != null) {
            mRawShippingOptions =
                    Collections.unmodifiableList(Arrays.asList(details.shippingOptions));
        } else if (mRawShippingOptions == null) {
            mRawShippingOptions = Collections.unmodifiableList(new ArrayList<>());
        }

        updateAppModifiedTotals();

        assert mRawTotal != null;
        assert mRawLineItems != null;

        return true;
    }

    // Implement PaymentUIsManager.Delegate:
    @Override
    public void updateAppModifiedTotals() {
        if (!PaymentFeatureList.isEnabled(PaymentFeatureList.WEB_PAYMENTS_MODIFIERS)) return;
        if (mModifiers == null) return;
        if (mPaymentUIsManager.getPaymentMethodsSection() == null) return;

        for (int i = 0; i < mPaymentUIsManager.getPaymentMethodsSection().getSize(); i++) {
            PaymentApp app = (PaymentApp) mPaymentUIsManager.getPaymentMethodsSection().getItem(i);
            PaymentDetailsModifier modifier = getModifier(app);
            app.setModifiedTotal(modifier == null || modifier.total == null
                            ? null
                            : getOrCreateCurrencyFormatter(modifier.total.amount)
                                      .format(modifier.total.amount.value));
        }

        updateOrderSummary(
                (PaymentApp) mPaymentUIsManager.getPaymentMethodsSection().getSelectedItem());
    }

    /** Sets the modifier for the order summary based on the given app, if any. */
    private void updateOrderSummary(@Nullable PaymentApp app) {
        if (!PaymentFeatureList.isEnabled(PaymentFeatureList.WEB_PAYMENTS_MODIFIERS)) return;

        PaymentDetailsModifier modifier = getModifier(app);
        PaymentItem total = modifier == null ? null : modifier.total;
        if (total == null) total = mRawTotal;

        CurrencyFormatter formatter = getOrCreateCurrencyFormatter(total.amount);
        mUiShoppingCart.setTotal(new LineItem(total.label, formatter.getFormattedCurrencyCode(),
                formatter.format(total.amount.value), false /* isPending */));
        mUiShoppingCart.setAdditionalContents(modifier == null
                        ? null
                        : getLineItems(Arrays.asList(modifier.additionalDisplayItems)));
        if (mPaymentUIsManager.getPaymentRequestUI() != null) {
            mPaymentUIsManager.getPaymentRequestUI().updateOrderSummarySection(mUiShoppingCart);
        }
    }

    /** @return The first modifier that matches the given app, or null. */
    @Nullable
    private PaymentDetailsModifier getModifier(@Nullable PaymentApp app) {
        if (mModifiers == null || app == null) return null;
        // Makes a copy to ensure it is modifiable.
        Set<String> methodNames = new HashSet<>(app.getInstrumentMethodNames());
        methodNames.retainAll(mModifiers.keySet());
        if (methodNames.isEmpty()) return null;

        for (String methodName : methodNames) {
            PaymentDetailsModifier modifier = mModifiers.get(methodName);
            if (app.isValidForPaymentMethodData(methodName, modifier.methodData)) {
                return modifier;
            }
        }

        return null;
    }

    /**
     * Converts a list of payment items and returns their parsed representation.
     *
     * @param items The payment items to parse. Can be null.
     * @return A list of valid line items.
     */
    private List<LineItem> getLineItems(@Nullable List<PaymentItem> items) {
        // Line items are optional.
        if (items == null) return new ArrayList<>();

        List<LineItem> result = new ArrayList<>(items.size());
        for (int i = 0; i < items.size(); i++) {
            PaymentItem item = items.get(i);
            CurrencyFormatter formatter = getOrCreateCurrencyFormatter(item.amount);
            result.add(new LineItem(item.label,
                    isMixedOrChangedCurrency() ? formatter.getFormattedCurrencyCode() : "",
                    formatter.format(item.amount.value), item.pending));
        }

        return Collections.unmodifiableList(result);
    }

    /**
     * Converts a list of shipping options and returns their parsed representation.
     *
     * @param options The raw shipping options to parse. Can be null.
     * @return The UI representation of the shipping options.
     */
    private SectionInformation getShippingOptions(@Nullable PaymentShippingOption[] options) {
        // Shipping options are optional.
        if (options == null || options.length == 0) {
            return new SectionInformation(PaymentRequestUI.DataType.SHIPPING_OPTIONS);
        }

        List<EditableOption> result = new ArrayList<>();
        int selectedItemIndex = SectionInformation.NO_SELECTION;
        for (int i = 0; i < options.length; i++) {
            PaymentShippingOption option = options[i];
            CurrencyFormatter formatter = getOrCreateCurrencyFormatter(option.amount);
            String currencyPrefix = isMixedOrChangedCurrency()
                    ? formatter.getFormattedCurrencyCode() + "\u0020"
                    : "";
            result.add(new EditableOption(option.id, option.label,
                    currencyPrefix + formatter.format(option.amount.value), null));
            if (option.selected) selectedItemIndex = i;
        }

        return new SectionInformation(PaymentRequestUI.DataType.SHIPPING_OPTIONS, selectedItemIndex,
                Collections.unmodifiableList(result));
    }

    /**
     * Load required currency formatter for a given PaymentDetails.
     *
     * Note that the cache (mCurrencyFormatterMap) is not cleared for
     * updated payment details so as to indicate the currency has been changed.
     *
     * @param details The given payment details.
     */
    private void loadCurrencyFormattersForPaymentDetails(PaymentDetails details) {
        if (details.total != null) {
            getOrCreateCurrencyFormatter(details.total.amount);
        }

        if (details.displayItems != null) {
            for (PaymentItem item : details.displayItems) {
                getOrCreateCurrencyFormatter(item.amount);
            }
        }

        if (details.shippingOptions != null) {
            for (PaymentShippingOption option : details.shippingOptions) {
                getOrCreateCurrencyFormatter(option.amount);
            }
        }

        if (details.modifiers != null) {
            for (PaymentDetailsModifier modifier : details.modifiers) {
                if (modifier.total != null) getOrCreateCurrencyFormatter(modifier.total.amount);
                for (PaymentItem displayItem : modifier.additionalDisplayItems) {
                    getOrCreateCurrencyFormatter(displayItem.amount);
                }
            }
        }
    }

    private boolean isMixedOrChangedCurrency() {
        return mCurrencyFormatterMap.size() > 1;
    }

    /**
     * Gets currency formatter for a given PaymentCurrencyAmount,
     * creates one if no existing instance is found.
     *
     * @amount The given payment amount.
     */
    private CurrencyFormatter getOrCreateCurrencyFormatter(PaymentCurrencyAmount amount) {
        String key = amount.currency;
        CurrencyFormatter formatter = mCurrencyFormatterMap.get(key);
        if (formatter == null) {
            formatter = new CurrencyFormatter(amount.currency, Locale.getDefault());
            mCurrencyFormatterMap.put(key, formatter);
        }
        return formatter;
    }

    /**
     * Called to retrieve the data to show in the initial PaymentRequest UI.
     */
    @Override
    public void getDefaultPaymentInformation(Callback<PaymentInformation> callback) {
        mPaymentInformationCallback = callback;

        // mPaymentUIsManager.getPaymentRequestUI().show() is called only after request.show() is
        // called and all payment apps are ready.
        assert mIsCurrentPaymentRequestShowing;
        assert mIsFinishedQueryingPaymentApps;

        if (mWaitForUpdatedDetails) return;

        mHandler.post(() -> {
            if (mPaymentUIsManager.getPaymentRequestUI() != null) providePaymentInformation();
        });
    }

    private void providePaymentInformation() {
        // Do not display service worker payment apps summary in single line so as to display its
        // origin completely.
        mPaymentUIsManager.getPaymentMethodsSection()
                .setDisplaySelectedItemSummaryInSingleLineInNormalMode(
                        mPaymentUIsManager.getSelectedPaymentAppType()
                        != PaymentAppType.SERVICE_WORKER_APP);
        mPaymentInformationCallback.onResult(new PaymentInformation(mUiShoppingCart,
                mPaymentUIsManager.getShippingAddressesSection(), mUiShippingOptions,
                mPaymentUIsManager.getContactSection(),
                mPaymentUIsManager.getPaymentMethodsSection()));
        mPaymentInformationCallback = null;

        if (!mDidRecordShowEvent) {
            mDidRecordShowEvent = true;
            mJourneyLogger.setEventOccurred(Event.SHOWN);
            // Record the triggered transaction amount only when the total amount in details is
            // finalized (i.e. mWaitForUpdatedDetails == false). Otherwise it will get recorded when
            // the updated details become available.
            if (!mWaitForUpdatedDetails) {
                assert mRawTotal != null;
                mJourneyLogger.recordTransactionAmount(
                        mRawTotal.amount.currency, mRawTotal.amount.value, false /*completed*/);
            }
        }
    }

    @Override
    public void getShoppingCart(final Callback<ShoppingCart> callback) {
        mHandler.post(callback.bind(mUiShoppingCart));
    }

    @Override
    public void getSectionInformation(@PaymentRequestUI.DataType final int optionType,
            final Callback<SectionInformation> callback) {
        SectionInformation result = null;
        switch (optionType) {
            case PaymentRequestUI.DataType.SHIPPING_ADDRESSES:
                result = mPaymentUIsManager.getShippingAddressesSection();
                break;
            case PaymentRequestUI.DataType.SHIPPING_OPTIONS:
                result = mUiShippingOptions;
                break;
            case PaymentRequestUI.DataType.CONTACT_DETAILS:
                result = mPaymentUIsManager.getContactSection();
                break;
            case PaymentRequestUI.DataType.PAYMENT_METHODS:
                result = mPaymentUIsManager.getPaymentMethodsSection();
                break;
            default:
                assert false;
        }
        mHandler.post(callback.bind(result));
    }

    @Override
    @PaymentRequestUI.SelectionResult
    public int onSectionOptionSelected(@PaymentRequestUI.DataType int optionType,
            EditableOption option, Callback<PaymentInformation> callback) {
        PaymentRequestClient client = getClient();
        if (client == null) return SelectionResult.NONE;
        if (optionType == PaymentRequestUI.DataType.SHIPPING_ADDRESSES) {
            // Log the change of shipping address.
            mJourneyLogger.incrementSelectionChanges(Section.SHIPPING_ADDRESS);
            AutofillAddress address = (AutofillAddress) option;
            if (address.isComplete()) {
                mPaymentUIsManager.getShippingAddressesSection().setSelectedItem(option);
                startShippingAddressChangeNormalization(address);
            } else {
                editAddress(address);
            }
            mPaymentInformationCallback = callback;
            return PaymentRequestUI.SelectionResult.ASYNCHRONOUS_VALIDATION;
        } else if (optionType == PaymentRequestUI.DataType.SHIPPING_OPTIONS) {
            // This may update the line items.
            mUiShippingOptions.setSelectedItem(option);
            client.onShippingOptionChange(option.getIdentifier());
            mPaymentInformationCallback = callback;
            return PaymentRequestUI.SelectionResult.ASYNCHRONOUS_VALIDATION;
        } else if (optionType == PaymentRequestUI.DataType.CONTACT_DETAILS) {
            // Log the change of contact info.
            mJourneyLogger.incrementSelectionChanges(Section.CONTACT_INFO);
            AutofillContact contact = (AutofillContact) option;
            if (contact.isComplete()) {
                mPaymentUIsManager.getContactSection().setSelectedItem(option);
                if (!mWasRetryCalled) return PaymentRequestUI.SelectionResult.NONE;
                dispatchPayerDetailChangeEventIfNeeded(contact.toPayerDetail());
            } else {
                editContact(contact);
                if (!mWasRetryCalled) return PaymentRequestUI.SelectionResult.EDITOR_LAUNCH;
            }
            mPaymentInformationCallback = callback;
            return PaymentRequestUI.SelectionResult.ASYNCHRONOUS_VALIDATION;
        } else if (optionType == PaymentRequestUI.DataType.PAYMENT_METHODS) {
            if (shouldShowShippingSection()
                    && mPaymentUIsManager.getShippingAddressesSection() == null) {
                ChromeActivity activity = ChromeActivity.fromWebContents(mWebContents);
                assert activity != null;
                createShippingSection(activity, mAutofillProfiles);
            }
            if (shouldShowContactSection() && mPaymentUIsManager.getContactSection() == null) {
                ChromeActivity activity = ChromeActivity.fromWebContents(mWebContents);
                assert activity != null;
                mPaymentUIsManager.setContactSection(new ContactDetailsSection(
                        activity, mAutofillProfiles, mContactEditor, mJourneyLogger));
            }
            mPaymentUIsManager.getPaymentRequestUI().selectedPaymentMethodUpdated(
                    new PaymentInformation(mUiShoppingCart,
                            mPaymentUIsManager.getShippingAddressesSection(), mUiShippingOptions,
                            mPaymentUIsManager.getContactSection(),
                            mPaymentUIsManager.getPaymentMethodsSection()));
            PaymentApp paymentApp = (PaymentApp) option;
            if (paymentApp instanceof AutofillPaymentInstrument) {
                AutofillPaymentInstrument card = (AutofillPaymentInstrument) paymentApp;

                if (!card.isComplete()) {
                    editCard(card);
                    return PaymentRequestUI.SelectionResult.EDITOR_LAUNCH;
                }
            }
            // Log the change of payment method.
            mJourneyLogger.incrementSelectionChanges(Section.PAYMENT_METHOD);

            updateOrderSummary(paymentApp);
            mPaymentUIsManager.getPaymentMethodsSection().setSelectedItem(option);
        }

        return PaymentRequestUI.SelectionResult.NONE;
    }

    @Override
    @PaymentRequestUI.SelectionResult
    public int onSectionEditOption(@PaymentRequestUI.DataType int optionType, EditableOption option,
            Callback<PaymentInformation> callback) {
        if (optionType == PaymentRequestUI.DataType.SHIPPING_ADDRESSES) {
            editAddress((AutofillAddress) option);
            mPaymentInformationCallback = callback;

            return PaymentRequestUI.SelectionResult.ASYNCHRONOUS_VALIDATION;
        }

        if (optionType == PaymentRequestUI.DataType.CONTACT_DETAILS) {
            editContact((AutofillContact) option);
            return PaymentRequestUI.SelectionResult.EDITOR_LAUNCH;
        }

        if (optionType == PaymentRequestUI.DataType.PAYMENT_METHODS) {
            editCard((AutofillPaymentInstrument) option);
            return PaymentRequestUI.SelectionResult.EDITOR_LAUNCH;
        }

        assert false;
        return PaymentRequestUI.SelectionResult.NONE;
    }

    @Override
    @PaymentRequestUI.SelectionResult
    public int onSectionAddOption(
            @PaymentRequestUI.DataType int optionType, Callback<PaymentInformation> callback) {
        if (optionType == PaymentRequestUI.DataType.SHIPPING_ADDRESSES) {
            editAddress(null);
            mPaymentInformationCallback = callback;
            // Log the add of shipping address.
            mJourneyLogger.incrementSelectionAdds(Section.SHIPPING_ADDRESS);
            return PaymentRequestUI.SelectionResult.ASYNCHRONOUS_VALIDATION;
        } else if (optionType == PaymentRequestUI.DataType.CONTACT_DETAILS) {
            editContact(null);
            // Log the add of contact info.
            mJourneyLogger.incrementSelectionAdds(Section.CONTACT_INFO);
            return PaymentRequestUI.SelectionResult.EDITOR_LAUNCH;
        } else if (optionType == PaymentRequestUI.DataType.PAYMENT_METHODS) {
            editCard(null);
            // Log the add of credit card.
            mJourneyLogger.incrementSelectionAdds(Section.PAYMENT_METHOD);
            return PaymentRequestUI.SelectionResult.EDITOR_LAUNCH;
        }

        return PaymentRequestUI.SelectionResult.NONE;
    }

    @Override
    public boolean shouldShowShippingSection() {
        if (!mRequestShipping) return false;

        if (mPaymentUIsManager.getPaymentMethodsSection() == null) return true;

        PaymentApp selectedApp =
                (PaymentApp) mPaymentUIsManager.getPaymentMethodsSection().getSelectedItem();
        return selectedApp == null || !selectedApp.handlesShippingAddress();
    }

    @Override
    public boolean shouldShowContactSection() {
        PaymentApp selectedApp = (mPaymentUIsManager.getPaymentMethodsSection() == null)
                ? null
                : (PaymentApp) mPaymentUIsManager.getPaymentMethodsSection().getSelectedItem();
        if (mRequestPayerName && (selectedApp == null || !selectedApp.handlesPayerName())) {
            return true;
        }
        if (mRequestPayerPhone && (selectedApp == null || !selectedApp.handlesPayerPhone())) {
            return true;
        }
        if (mRequestPayerEmail && (selectedApp == null || !selectedApp.handlesPayerEmail())) {
            return true;
        }

        return false;
    }

    private void editAddress(final AutofillAddress toEdit) {
        if (toEdit != null) {
            // Log the edit of a shipping address.
            mJourneyLogger.incrementSelectionEdits(Section.SHIPPING_ADDRESS);
        }
        mPaymentUIsManager.getAddressEditor().edit(toEdit, new Callback<AutofillAddress>() {
            @Override
            public void onResult(AutofillAddress editedAddress) {
                if (mPaymentUIsManager.getPaymentRequestUI() == null) return;

                if (editedAddress != null) {
                    mPaymentUIsManager.getAddressEditor().setAddressErrors(null);

                    // Sets or updates the shipping address label.
                    editedAddress.setShippingAddressLabelWithCountry();

                    mPaymentUIsManager.getCardEditor().updateBillingAddressIfComplete(
                            editedAddress);

                    // A partial or complete address came back from the editor (could have been from
                    // adding/editing or cancelling out of the edit flow).
                    if (!editedAddress.isComplete()) {
                        // If the address is not complete, unselect it (editor can return incomplete
                        // information when cancelled).
                        mPaymentUIsManager.getShippingAddressesSection().setSelectedItemIndex(
                                SectionInformation.NO_SELECTION);
                        providePaymentInformation();
                    } else {
                        if (toEdit == null) {
                            // Address is complete and user was in the "Add flow": add an item to
                            // the list.
                            mPaymentUIsManager.getShippingAddressesSection().addAndSelectItem(
                                    editedAddress);
                        }

                        if (mPaymentUIsManager.getContactSection() != null) {
                            // Update |mPaymentUIsManager.getContactSection()| with the new/edited
                            // address, which will update an existing item or add a new one to the
                            // end of the list.
                            mPaymentUIsManager.getContactSection().addOrUpdateWithAutofillAddress(
                                    editedAddress);
                            mPaymentUIsManager.getPaymentRequestUI().updateSection(
                                    PaymentRequestUI.DataType.CONTACT_DETAILS,
                                    mPaymentUIsManager.getContactSection());
                        }

                        startShippingAddressChangeNormalization(editedAddress);
                    }
                } else {
                    providePaymentInformation();
                }

                if (!mRetryQueue.isEmpty()) mHandler.post(mRetryQueue.remove());
            }
        });
    }

    private void editContact(final AutofillContact toEdit) {
        if (toEdit != null) {
            // Log the edit of a contact info.
            mJourneyLogger.incrementSelectionEdits(Section.CONTACT_INFO);
        }
        mContactEditor.edit(toEdit, new Callback<AutofillContact>() {
            @Override
            public void onResult(AutofillContact editedContact) {
                if (mPaymentUIsManager.getPaymentRequestUI() == null) return;

                if (editedContact != null) {
                    mContactEditor.setPayerErrors(null);

                    // A partial or complete contact came back from the editor (could have been from
                    // adding/editing or cancelling out of the edit flow).
                    if (!editedContact.isComplete()) {
                        // If the contact is not complete according to the requirements of the flow,
                        // unselect it (editor can return incomplete information when cancelled).
                        mPaymentUIsManager.getContactSection().setSelectedItemIndex(
                                SectionInformation.NO_SELECTION);
                    } else if (toEdit == null) {
                        // Contact is complete and we were in the "Add flow": add an item to the
                        // list.
                        mPaymentUIsManager.getContactSection().addAndSelectItem(editedContact);
                    } else {
                        dispatchPayerDetailChangeEventIfNeeded(editedContact.toPayerDetail());
                    }
                    // If contact is complete and (toEdit != null), no action needed: the contact
                    // was already selected in the UI.
                }
                // If |editedContact| is null, the user has cancelled out of the "Add flow". No
                // action to take (if a contact was selected in the UI, it will stay selected).

                mPaymentUIsManager.getPaymentRequestUI().updateSection(
                        PaymentRequestUI.DataType.CONTACT_DETAILS,
                        mPaymentUIsManager.getContactSection());

                if (!mRetryQueue.isEmpty()) mHandler.post(mRetryQueue.remove());
            }
        });
    }

    private void editCard(final AutofillPaymentInstrument toEdit) {
        if (toEdit != null) {
            // Log the edit of a credit card.
            mJourneyLogger.incrementSelectionEdits(Section.PAYMENT_METHOD);
        }
        mPaymentUIsManager.getCardEditor().edit(toEdit, new Callback<AutofillPaymentInstrument>() {
            @Override
            public void onResult(AutofillPaymentInstrument editedCard) {
                if (mPaymentUIsManager.getPaymentRequestUI() == null) return;

                if (editedCard != null) {
                    // A partial or complete card came back from the editor (could have been from
                    // adding/editing or cancelling out of the edit flow).
                    if (!editedCard.isComplete()) {
                        // If the card is not complete, unselect it (editor can return incomplete
                        // information when cancelled).
                        mPaymentUIsManager.getPaymentMethodsSection().setSelectedItemIndex(
                                SectionInformation.NO_SELECTION);
                    } else if (toEdit == null) {
                        // Card is complete and we were in the "Add flow": add an item to the list.
                        mPaymentUIsManager.getPaymentMethodsSection().addAndSelectItem(editedCard);
                    }
                    // If card is complete and (toEdit != null), no action needed: the card was
                    // already selected in the UI.
                }
                // If |editedCard| is null, the user has cancelled out of the "Add flow". No action
                // to take (if another card was selected prior to the add flow, it will stay
                // selected).

                updateAppModifiedTotals();
                mPaymentUIsManager.getPaymentRequestUI().updateSection(
                        PaymentRequestUI.DataType.PAYMENT_METHODS,
                        mPaymentUIsManager.getPaymentMethodsSection());
            }
        });
    }

    @Override
    public void onInstrumentDetailsLoadingWithoutUI() {
        if (getClient() == null || mPaymentUIsManager.getPaymentRequestUI() == null
                || mPaymentResponseHelper == null) {
            return;
        }

        assert mPaymentUIsManager.getSelectedPaymentAppType() == PaymentAppType.AUTOFILL;

        mPaymentUIsManager.getPaymentRequestUI().showProcessingMessage();
    }

    @Override
    public boolean onPayClicked(EditableOption selectedShippingAddress,
            EditableOption selectedShippingOption, EditableOption selectedPaymentMethod) {
        mInvokedPaymentApp = (PaymentApp) selectedPaymentMethod;

        EditableOption selectedContact = mPaymentUIsManager.getContactSection() != null
                ? mPaymentUIsManager.getContactSection().getSelectedItem()
                : null;
        mPaymentResponseHelper = new PaymentResponseHelper(selectedShippingAddress,
                selectedShippingOption, selectedContact, mInvokedPaymentApp, mPaymentOptions,
                mSkipToGPayHelper != null, this);

        // Create maps that are subsets of mMethodData and mModifiers, that contain the payment
        // methods supported by the selected payment app. If the intersection of method data
        // contains more than one payment method, the payment app is at liberty to choose (or have
        // the user choose) one of the methods.
        Map<String, PaymentMethodData> methodData = new HashMap<>();
        Map<String, PaymentDetailsModifier> modifiers = new HashMap<>();
        boolean isGooglePaymentApp = false;
        for (String paymentMethodName : mInvokedPaymentApp.getInstrumentMethodNames()) {
            if (mMethodData.containsKey(paymentMethodName)) {
                methodData.put(paymentMethodName, mMethodData.get(paymentMethodName));
            }
            if (mModifiers != null && mModifiers.containsKey(paymentMethodName)) {
                modifiers.put(paymentMethodName, mModifiers.get(paymentMethodName));
            }
            if (paymentMethodName.equals(MethodStrings.ANDROID_PAY)
                    || paymentMethodName.equals(MethodStrings.GOOGLE_PAY)) {
                isGooglePaymentApp = true;
            }
        }

        mInvokedPaymentApp.setPaymentHandlerHost(getPaymentHandlerHost());
        // Only native apps can use PaymentDetailsUpdateService.
        if (mInvokedPaymentApp.getPaymentAppType() == PaymentAppType.NATIVE_MOBILE_APP) {
            PaymentDetailsUpdateServiceHelper.getInstance().initialize(new PackageManagerDelegate(),
                    ((AndroidPaymentApp) mInvokedPaymentApp).packageName(),
                    this /* PaymentApp.PaymentRequestUpdateEventListener */);
        }

        // Create payment options for the invoked payment app.
        PaymentOptions paymentOptions = new PaymentOptions();
        paymentOptions.requestShipping =
                mRequestShipping && mInvokedPaymentApp.handlesShippingAddress();
        paymentOptions.requestPayerName =
                mRequestPayerName && mInvokedPaymentApp.handlesPayerName();
        paymentOptions.requestPayerPhone =
                mRequestPayerPhone && mInvokedPaymentApp.handlesPayerPhone();
        paymentOptions.requestPayerEmail =
                mRequestPayerEmail && mInvokedPaymentApp.handlesPayerEmail();
        paymentOptions.shippingType =
                mRequestShipping && mInvokedPaymentApp.handlesShippingAddress()
                ? mShippingType
                : PaymentShippingType.SHIPPING;

        // Redact shipping options if the selected app cannot handle shipping.
        List<PaymentShippingOption> redactedShippingOptions =
                mInvokedPaymentApp.handlesShippingAddress()
                ? mRawShippingOptions
                : Collections.unmodifiableList(new ArrayList<>());
        mInvokedPaymentApp.invokePaymentApp(mId, mMerchantName, mTopLevelOrigin,
                mPaymentRequestOrigin, mCertificateChain, Collections.unmodifiableMap(methodData),
                mRawTotal, mRawLineItems, Collections.unmodifiableMap(modifiers), paymentOptions,
                redactedShippingOptions, this);

        mJourneyLogger.setEventOccurred(Event.PAY_CLICKED);
        boolean isAutofillCard = mInvokedPaymentApp.isAutofillInstrument();
        // Record what type of app was selected when "Pay" was clicked.
        if (isAutofillCard) {
            mJourneyLogger.setEventOccurred(Event.SELECTED_CREDIT_CARD);
        } else if (isGooglePaymentApp) {
            mJourneyLogger.setEventOccurred(Event.SELECTED_GOOGLE);
        } else {
            mJourneyLogger.setEventOccurred(Event.SELECTED_OTHER);
        }
        return !isAutofillCard;
    }

    private PaymentHandlerHost getPaymentHandlerHost() {
        if (mPaymentHandlerHost == null) {
            mPaymentHandlerHost = new PaymentHandlerHost(mWebContents, /*delegate=*/this);
        }
        return mPaymentHandlerHost;
    }

    @Override
    public JourneyLogger getJourneyLogger() {
        return mJourneyLogger;
    }

    @Override
    public void onDismiss() {
        mJourneyLogger.setAborted(AbortReason.ABORTED_BY_USER);
        disconnectFromClientWithDebugMessage(ErrorStrings.USER_CANCELLED);
    }

    // Implement ComponentPaymentRequestDelegate:
    // This method is not supposed to be used outside this class and
    // ComponentPaymentRequestImpl.
    @Override
    public void disconnectFromClientWithDebugMessage(String debugMessage) {
        disconnectFromClientWithDebugMessage(debugMessage, PaymentErrorReason.USER_CANCEL);
    }

    private void disconnectFromClientWithDebugMessage(String debugMessage, int reason) {
        Log.d(TAG, debugMessage);
        PaymentRequestClient client = getClient();
        if (client != null) client.onError(reason, debugMessage);
        closeClient();
        closeUIAndDestroyNativeObjects();
        if (mNativeObserverForTest != null) mNativeObserverForTest.onConnectionTerminated();
    }

    // Implement ComponentPaymentRequestDelegate:
    /**
     * Called by the merchant website to abort the payment.
     */
    @Override
    public void abort() {
        if (getClient() == null) return;

        if (mInvokedPaymentApp != null) {
            mInvokedPaymentApp.abortPaymentApp(/*callback=*/this);
            return;
        }
        onInstrumentAbortResult(true);
    }

    /** Called by the payment app in response to an abort request. */
    @Override
    public void onInstrumentAbortResult(boolean abortSucceeded) {
        PaymentRequestClient client = getClient();
        if (client == null) return;
        client.onAbort(abortSucceeded);
        if (abortSucceeded) {
            closeClient();
            mJourneyLogger.setAborted(AbortReason.ABORTED_BY_MERCHANT);
            closeUIAndDestroyNativeObjects();
        } else {
            if (sObserverForTest != null) sObserverForTest.onPaymentRequestServiceUnableToAbort();
        }
        if (mNativeObserverForTest != null) mNativeObserverForTest.onAbortCalled();
    }

    // Implement ComponentPaymentRequestDelegate:
    /**
     * Called when the merchant website has processed the payment.
     */
    @Override
    public void complete(int result) {
        if (getClient() == null) return;

        if (result != PaymentComplete.FAIL) {
            mJourneyLogger.setCompleted();
            if (!PaymentPreferencesUtil.isPaymentCompleteOnce()) {
                PaymentPreferencesUtil.setPaymentCompleteOnce();
            }
            assert mRawTotal != null;
            mJourneyLogger.recordTransactionAmount(
                    mRawTotal.amount.currency, mRawTotal.amount.value, true /*completed*/);
        }

        /** Update records of the used payment app for sorting payment apps next time. */
        EditableOption selectedPaymentMethod =
                mPaymentUIsManager.getPaymentMethodsSection().getSelectedItem();
        PaymentPreferencesUtil.increasePaymentAppUseCount(selectedPaymentMethod.getIdentifier());
        PaymentPreferencesUtil.setPaymentAppLastUseDate(
                selectedPaymentMethod.getIdentifier(), System.currentTimeMillis());

        if (mMinimalUi != null) {
            if (result == PaymentComplete.FAIL) {
                mMinimalUi.showErrorAndClose(
                        this::onMinimalUiErroredAndClosed, R.string.payments_error_message);
            } else {
                mMinimalUi.showCompleteAndClose(this::onMinimalUiCompletedAndClosed);
            }
            return;
        }

        if (mNativeObserverForTest != null) {
            mNativeObserverForTest.onCompleteCalled();
        }

        closeUIAndDestroyNativeObjects();
    }

    // Implement ComponentPaymentRequestDelegate:
    @Override
    public void retry(PaymentValidationErrors errors) {
        if (getClient() == null) return;

        if (!PaymentValidator.validatePaymentValidationErrors(errors)) {
            mJourneyLogger.setAborted(AbortReason.INVALID_DATA_FROM_RENDERER);
            disconnectFromClientWithDebugMessage(ErrorStrings.INVALID_VALIDATION_ERRORS);
            return;
        }
        mSpec.retry(errors);

        mWasRetryCalled = true;

        // Remove all payment apps except the selected one.
        assert mPaymentUIsManager.getPaymentMethodsSection() != null;
        PaymentApp selectedApp =
                (PaymentApp) mPaymentUIsManager.getPaymentMethodsSection().getSelectedItem();
        assert selectedApp != null;
        mPaymentUIsManager.setPaymentMethodsSection(
                new SectionInformation(PaymentRequestUI.DataType.PAYMENT_METHODS,
                        /* selection = */ 0, new ArrayList<>(Arrays.asList(selectedApp))));
        mPaymentUIsManager.getPaymentRequestUI().updateSection(
                PaymentRequestUI.DataType.PAYMENT_METHODS,
                mPaymentUIsManager.getPaymentMethodsSection());
        mPaymentUIsManager.getPaymentRequestUI().disableAddingNewCardsDuringRetry();

        // Go back to the payment sheet
        mPaymentUIsManager.getPaymentRequestUI().onPayButtonProcessingCancelled();
        PaymentDetailsUpdateServiceHelper.getInstance().reset();
        if (!TextUtils.isEmpty(errors.error)) {
            mPaymentUIsManager.getPaymentRequestUI().setRetryErrorMessage(errors.error);
        } else {
            ChromeActivity activity = ChromeActivity.fromWebContents(mWebContents);
            mPaymentUIsManager.getPaymentRequestUI().setRetryErrorMessage(
                    activity.getResources().getString(R.string.payments_error_message));
        }

        if (shouldShowShippingSection() && hasShippingAddressError(errors.shippingAddress)) {
            mRetryQueue.add(() -> {
                mPaymentUIsManager.getAddressEditor().setAddressErrors(errors.shippingAddress);
                AutofillAddress selectedAddress =
                        (AutofillAddress) mPaymentUIsManager.getShippingAddressesSection()
                                .getSelectedItem();
                editAddress(selectedAddress);
            });
        }

        if (shouldShowContactSection() && hasPayerError(errors.payer)) {
            mRetryQueue.add(() -> {
                mContactEditor.setPayerErrors(errors.payer);
                AutofillContact selectedContact =
                        (AutofillContact) mPaymentUIsManager.getContactSection().getSelectedItem();
                editContact(selectedContact);
            });
        }

        if (!mRetryQueue.isEmpty()) mHandler.post(mRetryQueue.remove());
    }

    private boolean hasShippingAddressError(AddressErrors errors) {
        return !TextUtils.isEmpty(errors.addressLine) || !TextUtils.isEmpty(errors.city)
                || !TextUtils.isEmpty(errors.country)
                || !TextUtils.isEmpty(errors.dependentLocality)
                || !TextUtils.isEmpty(errors.organization) || !TextUtils.isEmpty(errors.phone)
                || !TextUtils.isEmpty(errors.postalCode) || !TextUtils.isEmpty(errors.recipient)
                || !TextUtils.isEmpty(errors.region) || !TextUtils.isEmpty(errors.sortingCode);
    }

    private boolean hasPayerError(PayerErrors errors) {
        return !TextUtils.isEmpty(errors.name) || !TextUtils.isEmpty(errors.phone)
                || !TextUtils.isEmpty(errors.email);
    }

    @Override
    public void onCardAndAddressSettingsClicked() {
        Context context = ChromeActivity.fromWebContents(mWebContents);
        if (context == null) {
            mJourneyLogger.setAborted(AbortReason.OTHER);
            disconnectFromClientWithDebugMessage(ErrorStrings.ACTIVITY_NOT_FOUND);
            return;
        }

        SettingsLauncher settingsLauncher = new SettingsLauncherImpl();
        settingsLauncher.launchSettingsActivity(context);
    }

    // Implement ComponentPaymentRequestDelegate:
    /** Called by the merchant website to check if the user has complete payment apps. */
    @Override
    public void canMakePayment() {
        if (getClient() == null) return;

        if (mNativeObserverForTest != null) mNativeObserverForTest.onCanMakePaymentCalled();

        if (mIsFinishedQueryingPaymentApps) {
            respondCanMakePaymentQuery();
        } else {
            mIsCanMakePaymentResponsePending = true;
        }
    }

    private void respondCanMakePaymentQuery() {
        PaymentRequestClient client = getClient();
        if (client == null) return;

        mIsCanMakePaymentResponsePending = false;

        boolean response = mCanMakePayment && mDelegate.prefsCanMakePayment();
        client.onCanMakePayment(response ? CanMakePaymentQueryResult.CAN_MAKE_PAYMENT
                                         : CanMakePaymentQueryResult.CANNOT_MAKE_PAYMENT);

        mJourneyLogger.setCanMakePaymentValue(response || mIsOffTheRecord);

        if (sObserverForTest != null) {
            sObserverForTest.onPaymentRequestServiceCanMakePaymentQueryResponded();
        }
        if (mNativeObserverForTest != null) mNativeObserverForTest.onCanMakePaymentReturned();
    }

    // Implement ComponentPaymentRequestDelegate:
    /** Called by the merchant website to check if the user has complete payment instruments. */
    @Override
    public void hasEnrolledInstrument(boolean perMethodQuota) {
        if (getClient() == null) return;

        if (mNativeObserverForTest != null) mNativeObserverForTest.onHasEnrolledInstrumentCalled();

        mHasEnrolledInstrumentUsesPerMethodQuota = perMethodQuota;

        if (mIsFinishedQueryingPaymentApps) {
            respondHasEnrolledInstrumentQuery(mHasEnrolledInstrument);
        } else {
            mIsHasEnrolledInstrumentResponsePending = true;
        }
    }

    private void respondHasEnrolledInstrumentQuery(boolean response) {
        PaymentRequestClient client = getClient();
        if (client == null) return;

        mIsHasEnrolledInstrumentResponsePending = false;

        if (CanMakePaymentQuery.canQuery(mWebContents, mTopLevelOrigin, mPaymentRequestOrigin,
                    mQueryForQuota, mHasEnrolledInstrumentUsesPerMethodQuota)) {
            client.onHasEnrolledInstrument(response
                            ? HasEnrolledInstrumentQueryResult.HAS_ENROLLED_INSTRUMENT
                            : HasEnrolledInstrumentQueryResult.HAS_NO_ENROLLED_INSTRUMENT);
        } else if (shouldEnforceCanMakePaymentQueryQuota()) {
            client.onHasEnrolledInstrument(HasEnrolledInstrumentQueryResult.QUERY_QUOTA_EXCEEDED);
        } else {
            client.onHasEnrolledInstrument(response
                            ? HasEnrolledInstrumentQueryResult.WARNING_HAS_ENROLLED_INSTRUMENT
                            : HasEnrolledInstrumentQueryResult.WARNING_HAS_NO_ENROLLED_INSTRUMENT);
        }

        mJourneyLogger.setHasEnrolledInstrumentValue(response || mIsOffTheRecord);

        if (sObserverForTest != null) {
            sObserverForTest.onPaymentRequestServiceHasEnrolledInstrumentQueryResponded();
        }
        if (mNativeObserverForTest != null) {
            mNativeObserverForTest.onHasEnrolledInstrumentReturned();
        }
    }

    /**
     * @return Whether canMakePayment() query quota should be enforced. By default, the quota is
     * enforced only on https:// scheme origins. However, the tests also enable the quota on
     * localhost and file:// scheme origins to verify its behavior.
     */
    private boolean shouldEnforceCanMakePaymentQueryQuota() {
        // If |mWebContents| is destroyed, don't bother checking the localhost or file:// scheme
        // exemption. It doesn't really matter anyways.
        return mWebContents.isDestroyed()
                || !UrlUtil.isLocalDevelopmentUrl(mWebContents.getLastCommittedUrl())
                || sIsLocalCanMakePaymentQueryQuotaEnforcedForTest;
    }

    // Implement ComponentPaymentRequestDelegate:
    /**
     * Called when the renderer closes the Mojo connection.
     */
    @Override
    public void close() {
        if (getClient() == null) return;
        closeClient();
        mJourneyLogger.setAborted(AbortReason.MOJO_RENDERER_CLOSING);
        if (sObserverForTest != null) sObserverForTest.onRendererClosedMojoConnection();
        closeUIAndDestroyNativeObjects();
        if (mNativeObserverForTest != null) mNativeObserverForTest.onConnectionTerminated();
    }

    // Implement ComponentPaymentRequestDelegate:
    /**
     * Called when the Mojo connection encounters an error.
     */
    @Override
    public void onConnectionError(MojoException e) {
        if (getClient() == null) return;
        closeClient();
        mJourneyLogger.setAborted(AbortReason.MOJO_CONNECTION_ERROR);
        closeUIAndDestroyNativeObjects();
        if (mNativeObserverForTest != null) mNativeObserverForTest.onConnectionTerminated();
    }

    // PaymentAppFactoryParams implementation.
    @Override
    public WebContents getWebContents() {
        return mWebContents;
    }

    // PaymentAppFactoryParams implementation.
    @Override
    public RenderFrameHost getRenderFrameHost() {
        return mRenderFrameHost;
    }

    // PaymentAppFactoryParams implementation.
    @Override
    public Map<String, PaymentMethodData> getMethodData() {
        return mMethodData;
    }

    // PaymentAppFactoryParams implementation.
    @Override
    public String getId() {
        return mId;
    }

    // PaymentAppFactoryParams implementation.
    @Override
    public String getTopLevelOrigin() {
        return mTopLevelOrigin;
    }

    // PaymentAppFactoryParams implementation.
    @Override
    public String getPaymentRequestOrigin() {
        return mPaymentRequestOrigin;
    }

    // PaymentAppFactoryParams implementation.
    @Override
    public Origin getPaymentRequestSecurityOrigin() {
        return mPaymentRequestSecurityOrigin;
    }

    // PaymentAppFactoryParams implementation.
    @Override
    @Nullable
    public byte[][] getCertificateChain() {
        return mCertificateChain;
    }

    // PaymentAppFactoryParams implementation.
    @Override
    public Map<String, PaymentDetailsModifier> getModifiers() {
        return mModifiers == null ? new HashMap<>() : Collections.unmodifiableMap(mModifiers);
    }

    // PaymentAppFactoryParams implementation.
    @Override
    public boolean getMayCrawl() {
        return !mPaymentUIsManager.canUserAddCreditCard()
                || PaymentFeatureList.isEnabledOrExperimentalFeaturesEnabled(
                        PaymentFeatureList.WEB_PAYMENTS_ALWAYS_ALLOW_JUST_IN_TIME_PAYMENT_APP);
    }

    // PaymentAppFactoryParams implementation.
    @Override
    public PaymentRequestUpdateEventListener getPaymentRequestUpdateEventListener() {
        return this;
    }

    // PaymentAppFactoryParams implementation.
    @Override
    public PaymentOptions getPaymentOptions() {
        return mPaymentOptions;
    }

    // PaymentAppFactoryParams implementation.
    @Override
    public PaymentRequestSpec getSpec() {
        return mSpec;
    }

    // PaymentAppFactoryDelegate implementation.
    @Override
    public PaymentAppFactoryParams getParams() {
        return this;
    }

    // PaymentAppFactoryDelegate implementation.
    @Override
    public void onCanMakePaymentCalculated(boolean canMakePayment) {
        if (getClient() == null) return;

        mCanMakePayment = canMakePayment;

        if (!mIsCanMakePaymentResponsePending) return;

        // canMakePayment doesn't need to wait for all apps to be queried because it only needs to
        // test the existence of a payment handler.
        respondCanMakePaymentQuery();
    }

    // PaymentAppFactoryDelegate implementation.
    @Override
    public void onAutofillPaymentAppCreatorAvailable(AutofillPaymentAppCreator creator) {
        mPaymentUIsManager.setAutofillPaymentAppCreator(creator);
    }

    // PaymentAppFactoryDelegate implementation.
    @Override
    public void onPaymentAppCreated(PaymentApp paymentApp) {
        if (getClient() == null) return;

        mHideServerAutofillCards |= paymentApp.isServerAutofillInstrumentReplacement();
        paymentApp.setHaveRequestedAutofillData(mHaveRequestedAutofillData);
        mHasEnrolledInstrument |= paymentApp.canMakePayment();
        mHasNonAutofillApp |= !paymentApp.isAutofillInstrument();

        if (paymentApp.isAutofillInstrument()) {
            mJourneyLogger.setEventOccurred(Event.AVAILABLE_METHOD_BASIC_CARD);
        } else if (paymentApp.getInstrumentMethodNames().contains(MethodStrings.GOOGLE_PAY)
                || paymentApp.getInstrumentMethodNames().contains(MethodStrings.ANDROID_PAY)) {
            mJourneyLogger.setEventOccurred(Event.AVAILABLE_METHOD_GOOGLE);
        } else {
            mJourneyLogger.setEventOccurred(Event.AVAILABLE_METHOD_OTHER);
        }

        mPendingApps.add(paymentApp);
    }

    // PaymentAppFactoryDelegate implementation.
    @Override
    public void onPaymentAppCreationError(String errorMessage) {
        if (TextUtils.isEmpty(mRejectShowErrorMessage)) mRejectShowErrorMessage = errorMessage;
    }

    // PaymentAppFactoryDelegate implementation.
    @Override
    public void onDoneCreatingPaymentApps(PaymentAppFactoryInterface factory /* Unused */) {
        mIsFinishedQueryingPaymentApps = true;

        if (getClient() == null || disconnectIfNoPaymentMethodsSupported()) {
            return;
        }

        // Always return false when can make payment is disabled.
        mHasEnrolledInstrument &= mDelegate.prefsCanMakePayment();

        if (mHideServerAutofillCards) {
            List<PaymentApp> nonServerAutofillCards = new ArrayList<>();
            int numberOfPendingApps = mPendingApps.size();
            for (int i = 0; i < numberOfPendingApps; i++) {
                if (!mPendingApps.get(i).isServerAutofillInstrument()) {
                    nonServerAutofillCards.add(mPendingApps.get(i));
                }
            }
            mPendingApps = nonServerAutofillCards;
        }

        // Load the validation rules for each unique region code in the credit card billing
        // addresses and check for validity.
        Set<String> uniqueCountryCodes = new HashSet<>();
        for (int i = 0; i < mPendingApps.size(); ++i) {
            @Nullable
            String countryCode = mPendingApps.get(i).getCountryCode();
            if (countryCode != null && !uniqueCountryCodes.contains(countryCode)) {
                uniqueCountryCodes.add(countryCode);
                PersonalDataManager.getInstance().loadRulesForAddressNormalization(countryCode);
            }
        }

        Collections.sort(mPendingApps, mPaymentAppComparator);

        // Possibly pre-select the first app on the list.
        int selection = !mPendingApps.isEmpty() && mPendingApps.get(0).canPreselect()
                ? 0
                : SectionInformation.NO_SELECTION;

        if (mIsCanMakePaymentResponsePending) {
            respondCanMakePaymentQuery();
        }

        if (mIsHasEnrolledInstrumentResponsePending) {
            respondHasEnrolledInstrumentQuery(mHasEnrolledInstrument);
        }

        ChromeActivity chromeActivity = ChromeActivity.fromWebContents(mWebContents);
        if (chromeActivity == null) {
            mJourneyLogger.setNotShown(NotShownReason.OTHER);
            disconnectFromClientWithDebugMessage(ErrorStrings.ACTIVITY_NOT_FOUND);
            if (sObserverForTest != null) sObserverForTest.onPaymentRequestServiceShowFailed();
            return;
        }

        // The list of payment apps is ready to display.
        mPaymentUIsManager.setPaymentMethodsSection(
                new SectionInformation(PaymentRequestUI.DataType.PAYMENT_METHODS, selection,
                        new ArrayList<>(mPendingApps)));

        // Record the number suggested payment methods and whether at least one of them was
        // complete.
        mJourneyLogger.setNumberOfSuggestionsShown(Section.PAYMENT_METHOD, mPendingApps.size(),
                !mPendingApps.isEmpty() && mPendingApps.get(0).isComplete());

        int missingFields = 0;
        if (mPendingApps.isEmpty()) {
            // TODO(crbug.com/1107039): This value could be null when this method is entered from
            // PaymentRequest#init. We should turn it into boolean after correcting this bug.
            Boolean merchantSupportsAutofillCards =
                    mPaymentUIsManager.merchantSupportsAutofillCards();
            if (merchantSupportsAutofillCards != null && merchantSupportsAutofillCards) {
                // Record all fields if basic-card is supported but no card exists.
                missingFields = AutofillPaymentInstrument.CompletionStatus.CREDIT_CARD_EXPIRED
                        | AutofillPaymentInstrument.CompletionStatus.CREDIT_CARD_NO_CARDHOLDER
                        | AutofillPaymentInstrument.CompletionStatus.CREDIT_CARD_NO_NUMBER
                        | AutofillPaymentInstrument.CompletionStatus.CREDIT_CARD_NO_BILLING_ADDRESS;
            }
        } else if (mPendingApps.get(0).isAutofillInstrument()) {
            missingFields = ((AutofillPaymentInstrument) (mPendingApps.get(0))).getMissingFields();
        }
        if (missingFields != 0) {
            RecordHistogram.recordSparseHistogram(
                    "PaymentRequest.MissingPaymentFields", missingFields);
        }

        mPendingApps.clear();

        updateAppModifiedTotals();

        SettingsAutofillAndPaymentsObserver.getInstance().registerObserver(mPaymentUIsManager);

        if (mIsCurrentPaymentRequestShowing) {
            // Send AppListReady signal when all apps are created and request.show() is called.
            if (mNativeObserverForTest != null) {
                mNativeObserverForTest.onAppListReady(
                        mPaymentUIsManager.getPaymentMethodsSection().getItems(), mRawTotal);
            }
            // Calculate skip ui and build ui only after all payment apps are ready and
            // request.show() is called.
            calculateWhetherShouldSkipShowingPaymentRequestUi();
            if (!buildUI(chromeActivity)) return;
            if (!mShouldSkipShowingPaymentRequestUi && mSkipToGPayHelper == null) {
                mPaymentUIsManager.getPaymentRequestUI().show();
            }
        }

        triggerPaymentAppUiSkipIfApplicable(chromeActivity);
    }

    /**
     * If no payment methods are supported, disconnect from the client and return true.
     * @return Whether client has been disconnected.
     */
    private boolean disconnectIfNoPaymentMethodsSupported() {
        if (!mIsFinishedQueryingPaymentApps || !mIsCurrentPaymentRequestShowing) return false;

        boolean havePaymentApps = !mPendingApps.isEmpty()
                || (mPaymentUIsManager.getPaymentMethodsSection() != null
                        && !mPaymentUIsManager.getPaymentMethodsSection().isEmpty());

        if (!mCanMakePayment
                || (!havePaymentApps && !mPaymentUIsManager.merchantSupportsAutofillCards())) {
            // All factories have responded, but none of them have apps. It's possible to add credit
            // cards, but the merchant does not support them either. The payment request must be
            // rejected.
            mJourneyLogger.setNotShown(mCanMakePayment
                            ? NotShownReason.NO_MATCHING_PAYMENT_METHOD
                            : NotShownReason.NO_SUPPORTED_PAYMENT_METHOD);
            if (mIsProhibitedOriginOrInvalidSsl) {
                if (mNativeObserverForTest != null) mNativeObserverForTest.onNotSupportedError();
                // Chrome always refuses payments with invalid SSL and in prohibited origin types.
                disconnectFromClientWithDebugMessage(
                        mRejectShowErrorMessage, PaymentErrorReason.NOT_SUPPORTED);
            } else if (mIsOffTheRecord) {
                // If the user is in the OffTheRecord mode, hide the absence of their payment
                // methods from the merchant site.
                disconnectFromClientWithDebugMessage(
                        ErrorStrings.USER_CANCELLED, PaymentErrorReason.USER_CANCEL);
            } else {
                if (mNativeObserverForTest != null) mNativeObserverForTest.onNotSupportedError();

                if (TextUtils.isEmpty(mRejectShowErrorMessage) && !isInTwa()
                        && mMethodData.get(MethodStrings.GOOGLE_PLAY_BILLING) != null) {
                    mRejectShowErrorMessage = ErrorStrings.APP_STORE_METHOD_ONLY_SUPPORTED_IN_TWA;
                }
                disconnectFromClientWithDebugMessage(
                        ErrorMessageUtil.getNotSupportedErrorMessage(mMethodData.keySet())
                                + (TextUtils.isEmpty(mRejectShowErrorMessage)
                                                ? ""
                                                : " " + mRejectShowErrorMessage),
                        PaymentErrorReason.NOT_SUPPORTED);
            }
            if (sObserverForTest != null) sObserverForTest.onPaymentRequestServiceShowFailed();
            return true;
        }

        return disconnectForStrictShow();
    }

    private boolean isInTwa() {
        return !TextUtils.isEmpty(
                mDelegate.getTwaPackageName(ChromeActivity.fromWebContents(mWebContents)));
    }

    /**
     * If strict show() conditions are not satisfied, disconnect from client and return true.
     * @return Whether client has been disconnected.
     */
    private boolean disconnectForStrictShow() {
        if (!mIsUserGestureShow || !mMethodData.containsKey(MethodStrings.BASIC_CARD)
                || mHasEnrolledInstrument || mHasNonAutofillApp
                || !PaymentFeatureList.isEnabledOrExperimentalFeaturesEnabled(
                        PaymentFeatureList.STRICT_HAS_ENROLLED_AUTOFILL_INSTRUMENT)) {
            return false;
        }

        if (sObserverForTest != null) sObserverForTest.onPaymentRequestServiceShowFailed();
        mRejectShowErrorMessage = ErrorStrings.STRICT_BASIC_CARD_SHOW_REJECT;
        disconnectFromClientWithDebugMessage(
                ErrorMessageUtil.getNotSupportedErrorMessage(mMethodData.keySet()) + " "
                        + mRejectShowErrorMessage,
                PaymentErrorReason.NOT_SUPPORTED);

        return true;
    }

    /** Called after retrieving payment details. */
    @Override
    public void onInstrumentDetailsReady(
            String methodName, String stringifiedDetails, PayerData payerData) {
        assert methodName != null;
        assert stringifiedDetails != null;

        if (getClient() == null || mPaymentResponseHelper == null) {
            return;
        }

        // If the payment method was an Autofill credit card with an identifier, record its use.
        PaymentApp selectedPaymentMethod =
                (PaymentApp) mPaymentUIsManager.getPaymentMethodsSection().getSelectedItem();
        if (selectedPaymentMethod != null
                && selectedPaymentMethod.getPaymentAppType() == PaymentAppType.AUTOFILL
                && !selectedPaymentMethod.getIdentifier().isEmpty()) {
            PersonalDataManager.getInstance().recordAndLogCreditCardUse(
                    selectedPaymentMethod.getIdentifier());
        }

        // Showing the payment request UI if we were previously skipping it so the loading
        // spinner shows up until the merchant notifies that payment was completed.
        if (mShouldSkipShowingPaymentRequestUi
                && mPaymentUIsManager.getPaymentRequestUI() != null) {
            mPaymentUIsManager.getPaymentRequestUI().showProcessingMessageAfterUiSkip();
        }

        mJourneyLogger.setEventOccurred(Event.RECEIVED_INSTRUMENT_DETAILS);

        mPaymentResponseHelper.onPaymentDetailsReceived(methodName, stringifiedDetails, payerData);
    }

    @Override
    public void onPaymentResponseReady(PaymentResponse response) {
        if (mSkipToGPayHelper != null && !mSkipToGPayHelper.patchPaymentResponse(response)) {
            disconnectFromClientWithDebugMessage(
                    ErrorStrings.PAYMENT_APP_INVALID_RESPONSE, PaymentErrorReason.NOT_SUPPORTED);
        }
        PaymentRequestClient client = getClient();
        if (client == null) return;
        client.onPaymentResponse(response);
        mPaymentResponseHelper = null;
        if (sObserverForTest != null) sObserverForTest.onPaymentResponseReady();
    }

    /** Called if unable to retrieve payment details. */
    @Override
    public void onInstrumentDetailsError(String errorMessage) {
        if (getClient() == null) return;
        mInvokedPaymentApp = null;
        if (mMinimalUi != null) {
            mJourneyLogger.setAborted(AbortReason.ABORTED_BY_USER);
            mMinimalUi.showErrorAndClose(
                    this::onMinimalUiErroredAndClosed, R.string.payments_error_message);
            return;
        }

        // When skipping UI, any errors/cancel from fetching payment details should abort payment.
        if (mShouldSkipShowingPaymentRequestUi) {
            assert !TextUtils.isEmpty(errorMessage);
            mJourneyLogger.setAborted(AbortReason.ABORTED_BY_USER);
            disconnectFromClientWithDebugMessage(errorMessage);
        } else {
            mPaymentUIsManager.getPaymentRequestUI().onPayButtonProcessingCancelled();
            PaymentDetailsUpdateServiceHelper.getInstance().reset();
        }
    }

    @Override
    public void onFocusChanged(@PaymentRequestUI.DataType int dataType, boolean willFocus) {
        assert dataType == PaymentRequestUI.DataType.SHIPPING_ADDRESSES;

        if (mPaymentUIsManager.getShippingAddressesSection().getSelectedItem() == null) return;

        AutofillAddress selectedAddress =
                (AutofillAddress) mPaymentUIsManager.getShippingAddressesSection()
                        .getSelectedItem();

        // The label should only include the country if the view is focused.
        if (willFocus) {
            selectedAddress.setShippingAddressLabelWithCountry();
        } else {
            selectedAddress.setShippingAddressLabelWithoutCountry();
        }

        mPaymentUIsManager.getPaymentRequestUI().updateSection(
                PaymentRequestUI.DataType.SHIPPING_ADDRESSES,
                mPaymentUIsManager.getShippingAddressesSection());
    }

    @Override
    public void onAddressNormalized(AutofillProfile profile) {
        PaymentRequestClient client = getClient();
        if (client == null) return;
        ChromeActivity chromeActivity = ChromeActivity.fromWebContents(mWebContents);

        // Can happen if the tab is closed during the normalization process.
        if (chromeActivity == null) {
            mJourneyLogger.setAborted(AbortReason.OTHER);
            disconnectFromClientWithDebugMessage(ErrorStrings.ACTIVITY_NOT_FOUND);
            if (sObserverForTest != null) sObserverForTest.onPaymentRequestServiceShowFailed();
            return;
        }

        // Don't reuse the selected address because it is formatted for display.
        AutofillAddress shippingAddress = new AutofillAddress(chromeActivity, profile);

        PaymentAddress redactedAddress = shippingAddress.toPaymentAddress();
        redactShippingAddress(redactedAddress);

        // This updates the line items and the shipping options asynchronously.
        client.onShippingAddressChange(redactedAddress);
    }

    @Override
    public void onCouldNotNormalize(AutofillProfile profile) {
        // Since the phone number is formatted in either case, this profile should be used.
        onAddressNormalized(profile);
    }

    /**
     * Starts the normalization of the new shipping address. Will call back into either
     * onAddressNormalized or onCouldNotNormalize which will send the result to the merchant.
     */
    private void startShippingAddressChangeNormalization(AutofillAddress address) {
        PersonalDataManager.getInstance().normalizeAddress(address.getProfile(), this);
    }

    private void ensureHideAndResetPaymentHandlerUi() {
        if (mPaymentHandlerUi == null) return;
        mPaymentHandlerUi.hide();
        mPaymentHandlerUi = null;
    }

    /**
     * Closes the UI and destroys native objects. If the client is still connected, then it's
     * notified of UI hiding. This PaymentRequestImpl object can't be reused after this function is
     * called.
     */
    private void closeUIAndDestroyNativeObjects() {
        ensureHideAndResetPaymentHandlerUi();
        if (mMinimalUi != null) {
            mMinimalUi.hide();
            mMinimalUi = null;
        }

        if (mPaymentUIsManager.getPaymentRequestUI() != null) {
            mPaymentUIsManager.getPaymentRequestUI().close();
            PaymentRequestClient client = getClient();
            if (client != null) {
                if (sObserverForTest != null) sObserverForTest.onCompleteReplied();
                client.onComplete();
                closeClient();
            }
            ChromeActivity activity = ChromeActivity.fromWebContents(mWebContents);
            if (activity != null) {
                activity.getLifecycleDispatcher().unregister(
                        mPaymentUIsManager.getPaymentRequestUI());
            }
            mPaymentUIsManager.setPaymentRequestUI(null);
            mPaymentUIsManager.getPaymentUisShowStateReconciler().onPaymentRequestUiClosed();
        }

        setShowingPaymentRequest(null);
        mIsCurrentPaymentRequestShowing = false;

        if (mPaymentUIsManager.getPaymentMethodsSection() != null) {
            for (int i = 0; i < mPaymentUIsManager.getPaymentMethodsSection().getSize(); i++) {
                EditableOption option = mPaymentUIsManager.getPaymentMethodsSection().getItem(i);
                ((PaymentApp) option).dismissInstrument();
            }
            mPaymentUIsManager.setPaymentMethodsSection(null);
        }

        if (mObservedTabModelSelector != null) {
            mObservedTabModelSelector.removeObserver(mSelectorObserver);
            mObservedTabModelSelector = null;
        }

        if (mObservedTabModel != null) {
            mObservedTabModel.removeObserver(mTabModelObserver);
            mObservedTabModel = null;
        }

        if (mOverviewModeBehavior != null) {
            mOverviewModeBehavior.removeOverviewModeObserver(mOverviewModeObserver);
            mOverviewModeBehavior = null;
        }

        SettingsAutofillAndPaymentsObserver.getInstance().unregisterObserver(mPaymentUIsManager);

        // Destroy native objects.
        for (CurrencyFormatter formatter : mCurrencyFormatterMap.values()) {
            assert formatter != null;
            // Ensures the native implementation of currency formatter does not leak.
            formatter.destroy();
        }
        mJourneyLogger.destroy();

        if (mPaymentHandlerHost != null) {
            mPaymentHandlerHost.destroy();
            mPaymentHandlerHost = null;
        }

        if (mSpec != null) {
            mSpec.destroy();
            mSpec = null;
        }
        PaymentDetailsUpdateServiceHelper.getInstance().reset();
    }

    private void dispatchPayerDetailChangeEventIfNeeded(PayerDetail detail) {
        PaymentRequestClient client = getClient();
        if (client == null || !mWasRetryCalled) return;
        client.onPayerDetailChange(detail);
    }

    /**
     * Redact shipping address before exposing it in ShippingAddressChangeEvent.
     * https://w3c.github.io/payment-request/#shipping-address-changed-algorithm
     * @param shippingAddress The shippingAddress to get redacted.
     */
    private void redactShippingAddress(PaymentAddress shippingAddress) {
        if (PaymentFeatureList.isEnabledOrExperimentalFeaturesEnabled(
                    PaymentFeatureList.WEB_PAYMENTS_REDACT_SHIPPING_ADDRESS)) {
            shippingAddress.organization = "";
            shippingAddress.phone = "";
            shippingAddress.recipient = "";
            shippingAddress.addressLine = new String[0];
        }
    }

    /**
     * @return Whether any instance of PaymentRequest has received a show() call.
     *         Don't use this function to check whether the current instance has
     *         received a show() call.
     */
    private static boolean getIsAnyPaymentRequestShowing() {
        return sShowingPaymentRequest != null;
    }

    /** @param paymentRequest The currently showing PaymentRequestImpl. */
    private static void setShowingPaymentRequest(PaymentRequestImpl paymentRequest) {
        assert sShowingPaymentRequest == null || paymentRequest == null;
        sShowingPaymentRequest = paymentRequest;
    }

    @VisibleForTesting
    public static void setObserverForTest(PaymentRequestServiceObserverForTest observerForTest) {
        sObserverForTest = observerForTest;
    }

    @VisibleForTesting
    public static void setIsLocalCanMakePaymentQueryQuotaEnforcedForTest() {
        sIsLocalCanMakePaymentQueryQuotaEnforcedForTest = true;
    }

    @VisibleForTesting
    /* package */ void setSkipUIForNonURLPaymentMethodIdentifiersForTest() {
        mSkipUiForNonUrlPaymentMethodIdentifiers = true;
    }

    @Nullable
    private PaymentRequestClient getClient() {
        return mComponentPaymentRequestImpl.getClient();
    }

    // Pre-condition: the client is not null.
    private void closeClient() {
        assert getClient() != null;
        mComponentPaymentRequestImpl.closeClient();
    }
}
