// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.signin;

import static androidx.test.espresso.Espresso.onView;
import static androidx.test.espresso.action.ViewActions.click;
import static androidx.test.espresso.action.ViewActions.pressBack;
import static androidx.test.espresso.assertion.ViewAssertions.doesNotExist;
import static androidx.test.espresso.assertion.ViewAssertions.matches;
import static androidx.test.espresso.matcher.ViewMatchers.Visibility.VISIBLE;
import static androidx.test.espresso.matcher.ViewMatchers.isDisplayed;
import static androidx.test.espresso.matcher.ViewMatchers.isRoot;
import static androidx.test.espresso.matcher.ViewMatchers.withEffectiveVisibility;
import static androidx.test.espresso.matcher.ViewMatchers.withId;
import static androidx.test.espresso.matcher.ViewMatchers.withText;

import static org.hamcrest.Matchers.not;
import static org.hamcrest.core.AllOf.allOf;
import static org.mockito.Mockito.verify;
import static org.mockito.MockitoAnnotations.initMocks;

import android.support.test.InstrumentationRegistry;

import androidx.test.filters.MediumTest;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.RuleChain;
import org.junit.runner.RunWith;
import org.mockito.Mock;

import org.chromium.base.test.util.CommandLineFlags;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.flags.ChromeFeatureList;
import org.chromium.chrome.browser.flags.ChromeSwitches;
import org.chromium.chrome.browser.signin.account_picker.AccountPickerBottomSheetCoordinator;
import org.chromium.chrome.browser.signin.account_picker.AccountPickerDelegate;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.chrome.test.ChromeTabbedActivityTestRule;
import org.chromium.chrome.test.util.browser.Features;
import org.chromium.chrome.test.util.browser.signin.AccountManagerTestRule;
import org.chromium.components.browser_ui.bottomsheet.BottomSheetController;
import org.chromium.components.signin.AccountManagerFacadeProvider;
import org.chromium.components.signin.ProfileDataSource;
import org.chromium.components.signin.test.util.FakeAccountManagerFacade;
import org.chromium.components.signin.test.util.FakeProfileDataSource;
import org.chromium.content_public.browser.test.util.TestThreadUtils;

/**
 * Tests account picker bottom sheet of the web signin flow.
 *
 * TODO(https://crbug.com/1090356): Add render tests for bottomsheet.
 */
@RunWith(ChromeJUnit4ClassRunner.class)
@CommandLineFlags.Add({ChromeSwitches.DISABLE_FIRST_RUN_EXPERIENCE})
@Features.EnableFeatures({ChromeFeatureList.MOBILE_IDENTITY_CONSISTENCY})
public class AccountPickerBottomSheetTest {
    private static class CustomFakeProfileDataSource extends FakeProfileDataSource {
        int getNumberOfObservers() {
            return mObservers.size();
        }
    }

    private static final ProfileDataSource.ProfileData PROFILE_DATA1 =
            new ProfileDataSource.ProfileData(
                    /* accountName= */ "test.account1@gmail.com", /* avatar= */ null,
                    /* fullName= */ "Test Account1", /* givenName= */ "Account1");
    private static final ProfileDataSource.ProfileData PROFILE_DATA2 =
            new ProfileDataSource.ProfileData(
                    /* accountName= */ "test.account2@gmail.com", /* avatar= */ null,
                    /* fullName= */ null, /* givenName= */ null);

    @Rule
    public final Features.InstrumentationProcessor mProcessor =
            new Features.InstrumentationProcessor();

    private final ChromeTabbedActivityTestRule mActivityTestRule =
            new ChromeTabbedActivityTestRule();

    private final CustomFakeProfileDataSource mFakeProfileDataSource =
            new CustomFakeProfileDataSource();

    private final AccountManagerTestRule mAccountManagerTestRule =
            new AccountManagerTestRule(mFakeProfileDataSource);

    // Destroys the mock AccountManagerFacade in the end as ChromeActivity may needs
    // to unregister observers in the stub.
    @Rule
    public final RuleChain mRuleChain =
            RuleChain.outerRule(mAccountManagerTestRule).around(mActivityTestRule);

    @Mock
    private AccountPickerDelegate mAccountPickerDelegateMock;

    @Before
    public void setUp() {
        initMocks(this);
        mAccountManagerTestRule.addAccount(PROFILE_DATA1);
        mAccountManagerTestRule.addAccount(PROFILE_DATA2);
        mActivityTestRule.startMainActivityOnBlankPage();
    }

    @Test
    @MediumTest
    public void testCollapsedSheetWithAccount() {
        buildAndShowCollapsedBottomSheet();
        checkCollapsedAccountList(PROFILE_DATA1);
    }

    @Test
    @MediumTest
    public void testExpandedSheet() {
        buildAndShowExpandedBottomSheet();
        onView(allOf(withText(PROFILE_DATA1.getAccountName()), withEffectiveVisibility(VISIBLE)))
                .check(matches(isDisplayed()));
        onView(allOf(withText(PROFILE_DATA1.getFullName()), withEffectiveVisibility(VISIBLE)))
                .check(matches(isDisplayed()));
        onView(withText(PROFILE_DATA2.getAccountName())).check(matches(isDisplayed()));
        onView(withText(R.string.signin_add_account_to_device)).check(matches(isDisplayed()));

        onView(withId(R.id.account_picker_selected_account)).check(matches(not(isDisplayed())));
        onView(withId(R.id.account_picker_continue_as_button)).check(matches(not(isDisplayed())));
    }

    @Test
    @MediumTest
    public void testCollapsedSheetWithZeroAccount() {
        // As we have already added accounts in our current AccountManagerFacade mock
        // Here since we want to test a zero account case, we would like to set up
        // a new AccountManagerFacade mock with no account in it. The mock will be
        // torn down in the end of the test in AccountManagerTestRule.
        AccountManagerFacadeProvider.setInstanceForTests(
                new FakeAccountManagerFacade(mFakeProfileDataSource));
        buildAndShowCollapsedBottomSheet();
        checkZeroAccountBottomSheet();
    }

    @Test
    @MediumTest
    public void testDismissCollapsedSheet() {
        buildAndShowCollapsedBottomSheet();
        onView(withText(PROFILE_DATA1.getAccountName())).check(matches(isDisplayed()));
        BottomSheetController controller = getBottomSheetController();
        Assert.assertTrue(controller.isSheetOpen());
        Assert.assertEquals(2, mFakeProfileDataSource.getNumberOfObservers());
        onView(isRoot()).perform(pressBack());
        Assert.assertFalse(controller.isSheetOpen());
        Assert.assertEquals(0, mFakeProfileDataSource.getNumberOfObservers());
    }

    @Test
    @MediumTest
    public void testDismissExpandedSheet() {
        buildAndShowExpandedBottomSheet();
        BottomSheetController controller = getBottomSheetController();
        Assert.assertTrue(controller.isSheetOpen());
        Assert.assertEquals(2, mFakeProfileDataSource.getNumberOfObservers());
        onView(isRoot()).perform(pressBack());
        Assert.assertFalse(controller.isSheetOpen());
        Assert.assertEquals(0, mFakeProfileDataSource.getNumberOfObservers());
    }

    @Test
    @MediumTest
    public void testAccountDisappearedInCollapsedSheet() {
        buildAndShowCollapsedBottomSheet();
        mAccountManagerTestRule.removeAccountAndWaitForSeeding(PROFILE_DATA1.getAccountName());
        mAccountManagerTestRule.removeAccountAndWaitForSeeding(PROFILE_DATA2.getAccountName());
        InstrumentationRegistry.getInstrumentation().waitForIdleSync();
        checkZeroAccountBottomSheet();
    }

    @Test
    @MediumTest
    public void testAccountDisappearedInExpandedSheet() {
        buildAndShowExpandedBottomSheet();
        mAccountManagerTestRule.removeAccountAndWaitForSeeding(PROFILE_DATA1.getAccountName());
        mAccountManagerTestRule.removeAccountAndWaitForSeeding(PROFILE_DATA2.getAccountName());
        InstrumentationRegistry.getInstrumentation().waitForIdleSync();
        checkZeroAccountBottomSheet();
    }

    @Test
    @MediumTest
    public void testAccountReappearedInCollapsedSheet() {
        mAccountManagerTestRule.removeAccountAndWaitForSeeding(PROFILE_DATA1.getAccountName());
        mAccountManagerTestRule.removeAccountAndWaitForSeeding(PROFILE_DATA2.getAccountName());
        buildAndShowCollapsedBottomSheet();
        checkZeroAccountBottomSheet();

        mAccountManagerTestRule.addAccount(PROFILE_DATA1.getAccountName());
        InstrumentationRegistry.getInstrumentation().waitForIdleSync();
        checkCollapsedAccountList(PROFILE_DATA1);
    }

    @Test
    @MediumTest
    public void testOtherAccountsChangeInCollapsedSheet() {
        buildAndShowCollapsedBottomSheet();
        checkCollapsedAccountList(PROFILE_DATA1);
        mAccountManagerTestRule.removeAccountAndWaitForSeeding(PROFILE_DATA2.getAccountName());
        InstrumentationRegistry.getInstrumentation().waitForIdleSync();
        checkCollapsedAccountList(PROFILE_DATA1);
    }

    @Test
    @MediumTest
    public void testSelectedAccountChangeInCollapsedSheet() {
        buildAndShowCollapsedBottomSheet();
        mAccountManagerTestRule.removeAccountAndWaitForSeeding(PROFILE_DATA1.getAccountName());
        InstrumentationRegistry.getInstrumentation().waitForIdleSync();
        checkCollapsedAccountList(PROFILE_DATA2);
    }

    @Test
    @MediumTest
    public void testProfileDataUpdateInExpandedSheet() {
        buildAndShowExpandedBottomSheet();
        String newFullName = "New Full Name1";
        String newGivenName = "New Given Name1";
        TestThreadUtils.runOnUiThreadBlocking(() -> {
            mFakeProfileDataSource.setProfileData(PROFILE_DATA1.getAccountName(),
                    new ProfileDataSource.ProfileData(
                            PROFILE_DATA1.getAccountName(), null, newFullName, newGivenName));
        });
        onView(allOf(withText(newFullName), withEffectiveVisibility(VISIBLE)))
                .check(matches(isDisplayed()));
        // Check that profile data update when the bottom sheet is expanded won't
        // toggle out any hidden part.
        onView(withId(R.id.account_picker_selected_account)).check(matches(not(isDisplayed())));
        onView(withId(R.id.account_picker_continue_as_button)).check(matches(not(isDisplayed())));
    }

    @Test
    @MediumTest
    public void testSignInDefaultAccountOnCollapsedSheet() {
        buildAndShowCollapsedBottomSheet();
        String continueAsText = mActivityTestRule.getActivity().getString(
                R.string.signin_promo_continue_as, PROFILE_DATA1.getGivenName());
        onView(withText(continueAsText)).perform(click());
        verify(mAccountPickerDelegateMock).signIn(PROFILE_DATA1.getAccountName());
    }

    @Test
    @MediumTest
    public void testSignInAnotherAccount() {
        buildAndShowExpandedBottomSheet();
        onView(withText(PROFILE_DATA2.getAccountName())).perform(click());
        String continueAsText = mActivityTestRule.getActivity().getString(
                R.string.signin_promo_continue_as, PROFILE_DATA2.getAccountName());
        onView(withText(continueAsText)).perform(click());
        verify(mAccountPickerDelegateMock).signIn(PROFILE_DATA2.getAccountName());
    }

    @Test
    @MediumTest
    public void testAddAccountOnExpandedSheet() {
        buildAndShowExpandedBottomSheet();
        onView(withText(R.string.signin_add_account_to_device)).perform(click());
        verify(mAccountPickerDelegateMock).addAccount();
    }

    @Test
    @MediumTest
    public void testSelectAnotherAccountOnExpandedSheet() {
        buildAndShowExpandedBottomSheet();
        onView(withText(PROFILE_DATA2.getAccountName())).perform(click());
        checkCollapsedAccountList(PROFILE_DATA2);
    }

    @Test
    @MediumTest
    public void testSelectTheSameAccountOnExpandedSheet() {
        buildAndShowExpandedBottomSheet();
        onView(allOf(withText(PROFILE_DATA1.getAccountName()), withEffectiveVisibility(VISIBLE)))
                .perform(click());
        checkCollapsedAccountList(PROFILE_DATA1);
    }

    private void checkZeroAccountBottomSheet() {
        onView(allOf(withText(PROFILE_DATA1.getAccountName()), withEffectiveVisibility(VISIBLE)))
                .check(doesNotExist());
        onView(allOf(withText(PROFILE_DATA2.getAccountName()), withEffectiveVisibility(VISIBLE)))
                .check(doesNotExist());
        onView(withId(R.id.account_picker_account_list)).check(matches(not(isDisplayed())));
        onView(withId(R.id.account_picker_selected_account)).check(matches(not(isDisplayed())));
        onView(allOf(withText(R.string.signin_add_account_to_device),
                       withEffectiveVisibility(VISIBLE)))
                .perform(click());
        verify(mAccountPickerDelegateMock).addAccount();
    }

    private void checkCollapsedAccountList(ProfileDataSource.ProfileData profileData) {
        onView(withText(R.string.signin_account_picker_dialog_title)).check(matches(isDisplayed()));
        onView(allOf(withText(profileData.getAccountName()), withEffectiveVisibility(VISIBLE)))
                .check(matches(isDisplayed()));
        if (profileData.getFullName() != null) {
            onView(allOf(withText(profileData.getFullName()), withEffectiveVisibility(VISIBLE)))
                    .check(matches(isDisplayed()));
        }
        onView(allOf(withId(R.id.account_selection_mark), withEffectiveVisibility(VISIBLE)))
                .check(matches(isDisplayed()));
        String continueAsText =
                mActivityTestRule.getActivity().getString(R.string.signin_promo_continue_as,
                        profileData.getGivenName() != null ? profileData.getGivenName()
                                                           : profileData.getAccountName());
        onView(withText(continueAsText)).check(matches(isDisplayed()));
        onView(withId(R.id.account_picker_account_list)).check(matches(not(isDisplayed())));
    }

    private void buildAndShowCollapsedBottomSheet() {
        TestThreadUtils.runOnUiThreadBlocking(() -> {
            AccountPickerBottomSheetCoordinator accountPickerBottomSheetCoordinator =
                    new AccountPickerBottomSheetCoordinator(mActivityTestRule.getActivity(),
                            getBottomSheetController(), mAccountPickerDelegateMock);
        });
        InstrumentationRegistry.getInstrumentation().waitForIdleSync();
    }

    private void buildAndShowExpandedBottomSheet() {
        buildAndShowCollapsedBottomSheet();
        onView(withText(PROFILE_DATA1.getFullName())).perform(click());
    }

    private BottomSheetController getBottomSheetController() {
        return mActivityTestRule.getActivity()
                .getRootUiCoordinatorForTesting()
                .getBottomSheetController();
    }
}
