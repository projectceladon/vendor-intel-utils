/*
 * Copyright (c) 2019 Intel Corporation.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.intel.multicamera;

import android.content.Context;
import android.content.SharedPreferences;
import android.content.SharedPreferences.OnSharedPreferenceChangeListener;
import android.graphics.ImageFormat;
import android.hardware.camera2.CameraAccessException;
import android.hardware.camera2.CameraCharacteristics;
import android.hardware.camera2.CameraManager;
import android.hardware.camera2.params.StreamConfigurationMap;
import android.media.CamcorderProfile;
import android.os.Bundle;
import android.util.Log;
import android.util.Size;
import android.util.SparseArray;
import androidx.appcompat.app.ActionBar;
import androidx.appcompat.app.AppCompatActivity;
import androidx.preference.ListPreference;
import androidx.preference.Preference;
import androidx.preference.PreferenceFragment;
import androidx.preference.PreferenceGroup;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

public class SettingsActivity extends AppCompatActivity {
    private String TAG = "settings";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.settings_activity);

        ActionBar actionBar = getSupportActionBar();
        if (actionBar != null) {
            actionBar.setDisplayHomeAsUpEnabled(true);
        }

        CameraManager manager = (CameraManager)getSystemService(Context.CAMERA_SERVICE);

        try {
            GlobalVariable.numOfCameras = manager.getCameraIdList().length;
            Log.d(TAG, "Inside Settings Total Cameras: " + manager.getCameraIdList().length);
        } catch (CameraAccessException e) {
            e.printStackTrace();
        }

        if (GlobalVariable.numOfCameras != 0) getSupportedSize(manager);

        getFragmentManager()
                .beginTransaction()
                .replace(R.id.settings, new SettingsFragment())
                .commit();
    }

    public void getSupportedSize(CameraManager manager) {
        try {
            String camerId;

            GlobalVariable.camerId = manager.getCameraIdList()[0];

            camerId = GlobalVariable.camerId;

            CameraCharacteristics characteristics = manager.getCameraCharacteristics(camerId);

            StreamConfigurationMap map =
                    characteristics.get(CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP);
            if (map == null) {
            }

            GlobalVariable.SupportedSizes =
                    new ArrayList<>(Arrays.asList(map.getOutputSizes(ImageFormat.JPEG)));

        } catch (CameraAccessException e) {
            e.printStackTrace();
        }
    }

    public static class SettingsFragment
            extends PreferenceFragment implements OnSharedPreferenceChangeListener {
        public String TAG = "SettingsFragment";
        public static final String SIZE_LARGE = "large";
        public static final String SIZE_MEDIUM = "medium";
        public static final String SIZE_SMALL = "small";

        public static String DEFAULT_KEY = "capture_list";

        public String[] mCamcorderProfileNames;
        private static final String SIZE_SETTING_STRING_DIMENSION_DELIMITER = "x";

        public static SparseArray<SelectedVideoQualities> sCachedSelectedVideoQualities =
                new SparseArray<SelectedVideoQualities>(3);
        private static String mPrefChangedKey = null;
        static boolean isPrefChangedKeyChanged = false;
        /** The selected {@link CamcorderProfile} qualities. */
        public static class SelectedVideoQualities {
            public int large = -1;
            public int medium = -1;
            public int small = -1;

            public int getFromSetting(String sizeSetting) {
                // Sanitize the value to be either small, medium or large. Default
                // to the latter.
                if (!SIZE_SMALL.equals(sizeSetting) && !SIZE_MEDIUM.equals(sizeSetting)) {
                    sizeSetting = SIZE_LARGE;
                }

                if (SIZE_LARGE.equals(sizeSetting)) {
                    return large;
                } else if (SIZE_MEDIUM.equals(sizeSetting)) {
                    return medium;
                } else {
                    return small;
                }
            }
        }

        /** Video qualities sorted by size. */
        public static int[] sVideoQualities =
                new int[] {// CamcorderProfile.QUALITY_HIGH,
                           CamcorderProfile.QUALITY_1080P, CamcorderProfile.QUALITY_720P,
                           CamcorderProfile.QUALITY_480P,  CamcorderProfile.QUALITY_CIF,
                           CamcorderProfile.QUALITY_QVGA,  CamcorderProfile.QUALITY_QCIF,
                           CamcorderProfile.QUALITY_LOW};

        static SelectedVideoQualities VideoQualities;
        public int numOfCameras = GlobalVariable.numOfCameras;
        public String camerId = GlobalVariable.camerId;
        ;
        public List<Size> PictureSizes = GlobalVariable.SupportedSizes;

        @Override
        public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
            setPreferencesFromResource(R.xml.root_preferences, rootKey);
            mCamcorderProfileNames = getResources().getStringArray(R.array.camcorder_profile_names);

            isPrefChangedKeyChanged = false;
        }

        @Override
        public void onResume() {
            super.onResume();
            // final Activity activity = this.getActivity();

            Log.d(TAG, "SettingsFragment onResume");

            /* Put in the summaries for the currently set values.
            final PreferenceScreen Multi_Cam =
                (PreferenceScreen) findPreference("multi_usb_cam_list");*/

            setVisibilities();

            if (numOfCameras > 0) {
                VideoQualities = getSelectedVideoQualities(0);

                // Put in the summaries for the currently set values.
                final PreferenceGroup Prf_Resolution =
                        (PreferenceGroup)findPreference("pref_resolution");

                fillEntriesAndSummaries(Prf_Resolution);
            }

            Log.d(TAG, "SettingsFragment onResume end");

            getPreferenceScreen().getSharedPreferences().registerOnSharedPreferenceChangeListener(
                    this);
        }

        @Override
        public void onPause() {
            super.onPause();
            getPreferenceScreen().getSharedPreferences().unregisterOnSharedPreferenceChangeListener(
                    this);
        }

        @Override
        public void onSharedPreferenceChanged(SharedPreferences sharedPreferences, String Key) {
            setSummary(findPreference(Key));
            mPrefChangedKey = Key;
            isPrefChangedKeyChanged = true;
        }

        public static String getchangedPrefKey() {
            if (isPrefChangedKeyChanged == true) {
                return mPrefChangedKey;
            } else {
                return DEFAULT_KEY;
            }
        }

        /**
         * Set the summary for the given preference. The given preference needs
         * to be a {@link ListPreference}.
         */
        private void setSummary(Preference preference) {
            if (!(preference instanceof ListPreference)) {
                return;
            }

            ListPreference listPreference = (ListPreference)preference;
            if (listPreference.getKey().equals("capture_list")) {
                setSummaryForSelection(PictureSizes, listPreference);
            } else if (listPreference.getKey().equals("video_list")) {
                setSummaryForSelection(VideoQualities, listPreference);
            } else {
                listPreference.setSummary(listPreference.getEntry());
            }
        }

        /**
         * This is used to serialize a size to a string for storage in settings
         *
         * @param size The size to serialize.
         * @return the string to be saved in preferences
         */
        private static String sizeToSettingString(Size size) {
            return size.getWidth() + SIZE_SETTING_STRING_DIMENSION_DELIMITER + size.getHeight();
        }

        /**
         * Sets the summary for the given list preference.
         *
         * @param displayableSizes The human readable preferred sizes
         * @param preference The preference for which to set the summary.
         */
        private void setSummaryForSelection(List<Size> displayableSizes,
                                            ListPreference preference) {
            String setting = preference.getValue();
            if (setting == null || !setting.contains("x")) {
                return;
            }
            Size settingSize = sizeFromSettingString(setting);
            if (settingSize == null) {
                return;
            }
            preference.setSummary(getSizeSummaryString(settingSize));
        }

        /**
         * Sets the summary for the given list preference.
         *
         * @param selectedQualities The selected video qualities.
         * @param preference The preference for which to set the summary.
         */
        private void setSummaryForSelection(SelectedVideoQualities selectedQualities,
                                            ListPreference preference) {
            if (selectedQualities == null) {
                return;
            }

            int selectedQuality = selectedQualities.getFromSetting(preference.getValue());
            Log.d(TAG, "SettingsFragment selectedQuality " + selectedQuality);
            preference.setSummary(mCamcorderProfileNames[selectedQuality]);
        }

        /**
         * This parses a setting string and returns the representative size.
         *
         * @param sizeSettingString The string that stored in settings to represent a size.
         * @return the represented Size.
         */
        public static Size sizeFromSettingString(String sizeSettingString) {
            if (sizeSettingString == null) {
                return null;
            }
            String[] parts = sizeSettingString.split(SIZE_SETTING_STRING_DIMENSION_DELIMITER);
            if (parts.length != 2) {
                return null;
            }

            try {
                int width = Integer.parseInt(parts[0]);
                int height = Integer.parseInt(parts[1]);
                return new Size(width, height);
            } catch (NumberFormatException ex) {
                return null;
            }
        }

        /**
         * @param size The photo resolution.
         * @return A human readable and translated string for labeling the
         *         picture size in megapixels.
         */
        private String getSizeSummaryString(Size size) {
            int width = size.getWidth();
            int height = size.getHeight();
            String result = getResources().getString(R.string.setting_summary_width_and_height,
                                                     width, height);
            return result;
        }

        /**
         * Depending on camera availability on the device, this removes settings
         * for cameras the device doesn't have.
         */
        private void setVisibilities() {
            Log.d(TAG, "SettingsFragment setVisibilities");

            PreferenceGroup Prf_Resolution = (PreferenceGroup)findPreference("pref_resolution");
            PreferenceGroup Prf_Src = (PreferenceGroup)findPreference("pref_Source");
            if (numOfCameras == 0) {
                recursiveDelete(Prf_Resolution, findPreference("capture_list"));
                recursiveDelete(Prf_Resolution, findPreference("video_list"));

                recursiveDelete(Prf_Src, findPreference("multi_usb_cam_list"));
            }
        }

        /**
         * Recursively go through settings and fill entries and summaries of our
         * preferences.
         */

        private void fillEntriesAndSummaries(PreferenceGroup group) {
            for (int i = 0; i < group.getPreferenceCount(); ++i) {
                Preference pref = group.getPreference(i);
                if (pref instanceof PreferenceGroup) {
                    fillEntriesAndSummaries((PreferenceGroup)pref);
                }
                setSummary(pref);
                setEntries(pref);
            }
        }

        /**
         * Recursively traverses the tree from the given group as the route and
         * tries to delete the preference. Traversal stops once the preference
         * was found and removed.
         */
        private boolean recursiveDelete(PreferenceGroup group, Preference preference) {
            Log.d(TAG, "SettingsFragment recursiveDelete");

            if (group == null) {
                Log.d(TAG, "attempting to delete from null preference group");
                return false;
            }
            if (preference == null) {
                Log.d(TAG, "attempting to delete null preference");
                return false;
            }
            if (group.removePreference(preference)) {
                Log.d(TAG, "Removal was successful.");
                return true;
            }

            for (int i = 0; i < group.getPreferenceCount(); ++i) {
                Preference pref = group.getPreference(i);
                if (pref instanceof PreferenceGroup) {
                    if (recursiveDelete((PreferenceGroup)pref, preference)) {
                        return true;
                    }
                }
            }
            return false;
        }

        /**
         * Set the entries for the given preference. The given preference needs
         * to be a {@link ListPreference}
         */
        private void setEntries(Preference preference) {
            if (!(preference instanceof ListPreference)) {
                return;
            }

            ListPreference listPreference = (ListPreference)preference;
            if (listPreference.getKey().equals("capture_list")) {
                setEntriesForSelection(PictureSizes, listPreference);
            } else if (listPreference.getKey().equals("video_list")) {
                setEntriesForSelection(VideoQualities, listPreference);
            }
        }

        /**
         * Sets the entries for the given list preference.
         *
         * @param selectedSizes The possible S,M,L entries the user can choose
         *            from.
         * @param preference The preference to set the entries for.
         */
        private void setEntriesForSelection(List<Size> selectedSizes, ListPreference preference) {
            if (selectedSizes == null) {
                return;
            }

            String[] entries = new String[selectedSizes.size()];
            String[] entryValues = new String[selectedSizes.size()];
            for (int i = 0; i < selectedSizes.size(); i++) {
                Size size = selectedSizes.get(i);
                entries[i] = getSizeSummaryString(size);
                entryValues[i] = sizeToSettingString(size);
            }
            preference.setEntries(entries);
            preference.setEntryValues(entryValues);
        }

        /**
         * Sets the entries for the given list preference.
         *
         * @param selectedQualities The possible S,M,L entries the user can
         *            choose from.
         * @param preference The preference to set the entries for.
         */
        private void setEntriesForSelection(SelectedVideoQualities selectedQualities,
                                            ListPreference preference) {
            if (selectedQualities == null) {
                return;
            }

            // Avoid adding double entries at the bottom of the list which
            // indicates that not at least 3 qualities are supported.
            ArrayList<String> entries = new ArrayList<String>();
            entries.add(mCamcorderProfileNames[selectedQualities.large]);
            if (selectedQualities.medium != selectedQualities.large) {
                entries.add(mCamcorderProfileNames[selectedQualities.medium]);
            }
            if (selectedQualities.small != selectedQualities.medium) {
                entries.add(mCamcorderProfileNames[selectedQualities.small]);
            }
            preference.setEntries(entries.toArray(new String[0]));
        }

        /**
         * Determines the video quality for large/medium/small for the given camera.
         * Returns the one matching the given setting. Defaults to 'large' of the
         * qualitySetting does not match either large. medium or small.
         *
         * @param qualitySetting One of 'large', 'medium', 'small'.
         * @param cameraId The ID of the camera for which to get the quality
         *            setting.
         * @return The CamcorderProfile quality setting.}
         */

        static int getVideoQuality(int cameraId, String qualitySetting) {
            return getSelectedVideoQualities(cameraId).getFromSetting(qualitySetting);
        }

        static SelectedVideoQualities getSelectedVideoQualities(int cameraId) {
            if (sCachedSelectedVideoQualities.get(cameraId) != null) {
                return sCachedSelectedVideoQualities.get(cameraId);
            }

            // Go through the sizes in descending order, see if they are supported,
            // and set large/medium/small accordingly.
            // If no quality is supported at all, the first call to
            // getNextSupportedQuality will throw an exception.
            // If only one quality is supported, then all three selected qualities
            // will be the same.
            int largeIndex = getNextSupportedVideoQualityIndex(cameraId, -1);
            int mediumIndex = getNextSupportedVideoQualityIndex(cameraId, largeIndex);
            int smallIndex = getNextSupportedVideoQualityIndex(cameraId, mediumIndex);
            VideoQualities = new SelectedVideoQualities();
            VideoQualities.large = sVideoQualities[largeIndex];
            VideoQualities.medium = sVideoQualities[mediumIndex];
            VideoQualities.small = sVideoQualities[smallIndex];
            sCachedSelectedVideoQualities.put(cameraId, VideoQualities);
            return VideoQualities;
        }

        /*
         * Starting from 'start' this method returns the next supported video
         * quality.
         */
        static int getNextSupportedVideoQualityIndex(int cameraId, int start) {
            for (int i = start + 1; i < sVideoQualities.length; ++i) {
                if (CamcorderProfile.hasProfile(cameraId, sVideoQualities[i])) {
                    // We found a new supported quality.
                    return i;
                }
            }

            // Failed to find another supported quality.
            if (start < 0 || start >= sVideoQualities.length) {
                // This means we couldn't find any supported quality.
                throw new IllegalArgumentException("Could not find supported video qualities.");
            }

            // We previously found a larger supported size. In this edge case, just
            // return the same index as the previous size.
            return start;
        }
    }
}
