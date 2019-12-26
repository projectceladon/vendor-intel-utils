/*
 * Copyright (C) 2014 The Android Open Source Project
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
import android.graphics.ImageFormat;
import android.hardware.camera2.CameraAccessException;
import android.hardware.camera2.CameraCharacteristics;
import android.hardware.camera2.CameraManager;
import android.hardware.camera2.params.StreamConfigurationMap;
import android.media.CamcorderProfile;
import android.os.Bundle;
import android.util.Log;
import android.util.Rational;
import android.util.Size;
import android.util.SparseArray;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import androidx.preference.ListPreference;
import androidx.preference.Preference;
import androidx.preference.PreferenceFragment;
import androidx.preference.PreferenceGroup;
import java.math.BigInteger;
import java.text.DecimalFormat;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

public class SettingsPrefUtil
        extends PreferenceFragment implements SharedPreferences.OnSharedPreferenceChangeListener {
    public static String TAG = "SettingsPrefUtil";

    public static final String SIZE_UHD4K = "UHD 4K";
    public static final String SIZE_FHD = "FHD 1080p";
    public static final String SIZE_HD = "HD 720p";
    public static final String SIZE_VGA = "SD 480p";

    public String[] mCamcorderProfileNames;
    private static final String SIZE_SETTING_STRING_DIMENSION_DELIMITER = "x";

    private String CameraId;
    private int root_preferences;
    private String pref_resolution;
    private String video_list, capture_list;
    public List<Size> PictureSizes;
    private ArrayList<String> VideoEntries;
    static final Size SIZE_4K = new Size(3840, 2160);
    static final Size SIZE_1080P = new Size(1920, 1080);
    static final Size SIZE_720P = new Size(1280, 720);
    static final Size SIZE_480P = new Size(640, 480);
    static final Size SIZE_240P = new Size(320, 240);

    public static int getFromSetting(String videoQuality) {
        // Sanitize the value to be either small, medium or large. Default
        // to the latter.

        if (SIZE_UHD4K.equals(videoQuality)) {
            return CamcorderProfile.QUALITY_2160P;
        } else if (SIZE_FHD.equals(videoQuality)) {
            return CamcorderProfile.QUALITY_1080P;
        } else if (SIZE_HD.equals(videoQuality)) {
            return CamcorderProfile.QUALITY_720P;
        } else if (SIZE_VGA.equals(videoQuality)) {
            return CamcorderProfile.QUALITY_480P;
        }

        return CamcorderProfile.QUALITY_480P;
    }

    public void getSupportedSize(String camerId) {
        try {
            List<Size> ImageDimentions;

            CameraManager manager =
                    (CameraManager)getActivity().getSystemService(Context.CAMERA_SERVICE);

            CameraCharacteristics characteristics = manager.getCameraCharacteristics(camerId);

            StreamConfigurationMap map =
                    characteristics.get(CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP);
            if (map == null) {
            }

            ImageDimentions = new ArrayList<>(Arrays.asList(map.getOutputSizes(ImageFormat.JPEG)));

            PictureSizes = new ArrayList<Size>(Arrays.<Size>asList());
            VideoEntries = new ArrayList<>();

            Log.d(TAG, " @@ getSupportedSize onResume @@" + CameraId);

            for (Size size : ImageDimentions) {
                if (size.equals(SIZE_240P)) {
                    PictureSizes.add(SIZE_240P);

                } else if (size.equals(SIZE_480P)) {
                    PictureSizes.add(SIZE_480P);
                    if (CamcorderProfile.hasProfile(0, sVideoQualities[3])) {
                        VideoEntries.add(mCamcorderProfileNames[sVideoQualities[3]]);
                    }

                } else if (size.equals(SIZE_720P)) {
                    PictureSizes.add(SIZE_720P);
                    if (CamcorderProfile.hasProfile(0, sVideoQualities[2])) {
                        VideoEntries.add(mCamcorderProfileNames[sVideoQualities[2]]);
                    }

                } else if (size.equals(SIZE_1080P)) {
                    PictureSizes.add(SIZE_1080P);
                    if (CamcorderProfile.hasProfile(0, sVideoQualities[1])) {
                        VideoEntries.add(mCamcorderProfileNames[sVideoQualities[1]]);
                    }

                } else if (size.equals(SIZE_4K)) {
                    PictureSizes.add(SIZE_4K);
                    if (CamcorderProfile.hasProfile(0, sVideoQualities[0])) {
                        VideoEntries.add(mCamcorderProfileNames[sVideoQualities[0]]);
                    }
                }
            }

        } catch (CameraAccessException e) {
            e.printStackTrace();
        }
    }

    /**
     * Video qualities sorted by size.
     */
    public static int[] sVideoQualities =
            new int[] {// CamcorderProfile.QUALITY_HIGH,
                       CamcorderProfile.QUALITY_2160P, CamcorderProfile.QUALITY_1080P,
                       CamcorderProfile.QUALITY_720P,  CamcorderProfile.QUALITY_480P,
                       CamcorderProfile.QUALITY_CIF,   CamcorderProfile.QUALITY_QVGA,
                       CamcorderProfile.QUALITY_QCIF,  CamcorderProfile.QUALITY_LOW};

    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
        switch (getArguments().getInt("root_preferences")) {
            case R.xml.root_preferences:
                root_preferences = R.xml.root_preferences;
                CameraId = getArguments().getString("Camera_id");
                break;
            case R.xml.root_preferences_1:
                root_preferences = R.xml.root_preferences_1;
                CameraId = getArguments().getString("Camera_id");
                break;
            case R.xml.root_preferences_2:
                root_preferences = R.xml.root_preferences_2;
                CameraId = getArguments().getString("Camera_id");
                break;
            case R.xml.root_preferences_3:
                root_preferences = R.xml.root_preferences_3;
                CameraId = getArguments().getString("Camera_id");
                break;
            default:
                break;
        }

        setPreferencesFromResource(root_preferences, rootKey);
        mCamcorderProfileNames = getResources().getStringArray(R.array.camcorder_profile_names);
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
                             Bundle savedInstanceState) {
        View v;
        v = super.onCreateView(inflater, container, savedInstanceState);

        switch (getArguments().getString("pref_resolution")) {
            case "pref_resolution":
                pref_resolution = "pref_resolution";
                break;
            case "pref_resolution_1":
                pref_resolution = "pref_resolution_1";
                break;
            case "pref_resolution_2":
                pref_resolution = "pref_resolution_2";
                break;
            case "pref_resolution_3":
                pref_resolution = "pref_resolution_3";
                break;
            default:
                break;
        }

        switch (getArguments().getString("video_list")) {
            case "video_list":
                video_list = "video_list";
                break;
            case "video_list_1":
                video_list = "video_list_1";
                break;
            case "video_list_2":
                video_list = "video_list_2";
                break;
            case "video_list_3":
                video_list = "video_list_3";
                break;
            default:
                break;
        }

        switch (getArguments().getString("capture_list")) {
            case "capture_list":
                capture_list = "capture_list";
                break;
            case "capture_list_1":
                capture_list = "capture_list_1";
                break;
            case "capture_list_2":
                capture_list = "capture_list_2";
                break;
            case "capture_list_3":
                capture_list = "capture_list_3";
                break;

            default:
                break;
        }

        getSupportedSize(CameraId);

        // Put in the summaries for the currently set values.
        final PreferenceGroup Prf_Resolution = (PreferenceGroup)findPreference(pref_resolution);

        fillEntriesAndSummaries(Prf_Resolution);

        return v;
    }

    @Override
    public void onResume() {
        super.onResume();

        Log.d(TAG, " @@ SettingsFragment onResume @@" + CameraId);

        getSupportedSize(CameraId);

        // Put in the summaries for the currently set values.
        final PreferenceGroup Prf_Resolution = (PreferenceGroup)findPreference(pref_resolution);

        fillEntriesAndSummaries(Prf_Resolution);

        getPreferenceScreen().getSharedPreferences().registerOnSharedPreferenceChangeListener(this);

        Log.d(TAG, "SettingsFragment onResume end");
    }

    @Override
    public void onPause() {
        super.onPause();

        Log.d(TAG, " @@ SettingsFragment onPause @@" + CameraId);
        getPreferenceScreen().getSharedPreferences().unregisterOnSharedPreferenceChangeListener(
                this);
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        Log.d(TAG, " @@ SettingsFragment onDestroy @@" + CameraId);
    }

    @Override
    public void onSharedPreferenceChanged(SharedPreferences sharedPreferences, String Key) {
        setSummary(findPreference(Key));

        switch (Key) {
            case "capture_list":
                sharedPreferences.edit().putString("pref_resolution", Key).apply();
                break;
            case "capture_list_1":
                sharedPreferences.edit().putString("pref_resolution_1", Key).apply();
                break;
            case "capture_list_2":
                sharedPreferences.edit().putString("pref_resolution_2", Key).apply();
                break;
            case "capture_list_3":
                sharedPreferences.edit().putString("pref_resolution_3", Key).apply();
                break;
            case "video_list":
                sharedPreferences.edit().putString("pref_resolution", Key).apply();
                break;
            case "video_list_1":
                sharedPreferences.edit().putString("pref_resolution_1", Key).apply();
                break;
            case "video_list_2":
                sharedPreferences.edit().putString("pref_resolution_2", Key).apply();
                break;
            case "video_list_3":
                sharedPreferences.edit().putString("pref_resolution_3", Key).apply();
                break;
            default:
                break;
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
        if (listPreference.getKey().equals(capture_list)) {
            setSummaryForSelection(PictureSizes, listPreference);
        } else if (listPreference.getKey().equals(video_list)) {
            setSummaryForSelection(listPreference);
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
     * @param preference       The preference for which to set the summary.
     */
    private void setSummaryForSelection(List<Size> displayableSizes, ListPreference preference) {
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
     *
     * @param preference        The preference for which to set the summary.
     */
    private void setSummaryForSelection(ListPreference preference) {
        if (preference == null) {
            return;
        }

        int selectedQuality = getFromSetting(preference.getValue());
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
     * Set the entries for the given preference. The given preference needs
     * to be a {@link ListPreference}
     */
    private void setEntries(Preference preference) {
        if (!(preference instanceof ListPreference)) {
            return;
        }

        ListPreference listPreference = (ListPreference)preference;
        if (listPreference.getKey().equals(capture_list)) {
            setEntriesForSelection(PictureSizes, listPreference);
        } else if (listPreference.getKey().equals(video_list)) {
            setEntriesForSelection(VideoEntries, listPreference);
        }
    }

    /**
     * Sets the entries for the given list preference.
     *
     * @param selectedSizes The possible S,M,L entries the user can choose
     *                      from.
     * @param preference    The preference to set the entries for.
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
     * @param entries The possible S,M,L entries the user can
     *                          choose from.
     * @param preference        The preference to set the entries for.
     */
    private void setEntriesForSelection(ArrayList<String> entries, ListPreference preference) {
        if (entries == null) {
            return;
        }

        preference.setEntries(entries.toArray(new String[0]));
    }

    /**
     * Different aspect ratio constants.
     */
    public static final Rational ASPECT_RATIO_16x9 = new Rational(16, 9);
    public static final Rational ASPECT_RATIO_4x3 = new Rational(4, 3);
    private static final double ASPECT_RATIO_TOLERANCE = 0.05;

    /**
     * These are the preferred aspect ratios for the settings. We will take HAL
     * supported aspect ratios that are within ASPECT_RATIO_TOLERANCE of these values.
     * We will also take the maximum supported resolution for full sensor image.
     */
    private static Float[] sDesiredAspectRatios = {16.0f / 9.0f, 4.0f / 3.0f};

    private static Size[] sDesiredAspectRatioSizes = {new Size(16, 9), new Size(4, 3)};

    /**
     * Take an aspect ratio and squish it into a nearby desired aspect ratio, if
     * possible.
     *
     * @param aspectRatio the aspect ratio to fuzz
     * @return the closest desiredAspectRatio within ASPECT_RATIO_TOLERANCE, or the
     *         original ratio
     */
    private static float fuzzAspectRatio(float aspectRatio) {
        for (float desiredAspectRatio : sDesiredAspectRatios) {
            if ((Math.abs(aspectRatio - desiredAspectRatio)) < ASPECT_RATIO_TOLERANCE) {
                return desiredAspectRatio;
            }
        }
        return aspectRatio;
    }

    /**
     * Reduce an aspect ratio to its lowest common denominator. The ratio of the
     * input and output sizes is guaranteed to be the same.
     *
     * @param aspectRatio the aspect ratio to reduce
     * @return The reduced aspect ratio which may equal the original.
     */
    public static Size reduce(Size aspectRatio) {
        BigInteger width = BigInteger.valueOf(aspectRatio.getWidth());
        BigInteger height = BigInteger.valueOf(aspectRatio.getHeight());
        BigInteger gcd = width.gcd(height);
        int numerator = Math.max(width.intValue(), height.intValue()) / gcd.intValue();
        int denominator = Math.min(width.intValue(), height.intValue()) / gcd.intValue();
        return new Size(numerator, denominator);
    }

    /**
     * Given a size, return the closest aspect ratio that falls close to the
     * given size.
     *
     * @param size the size to approximate
     * @return the closest desired aspect ratio, or the original aspect ratio if
     *         none were close enough
     */
    public static Size getApproximateSize(Size size) {
        Size aspectRatio = reduce(size);
        float fuzzy = fuzzAspectRatio(size.getWidth() / (float)size.getHeight());
        int index = Arrays.asList(sDesiredAspectRatios).indexOf(fuzzy);
        if (index != -1) {
            aspectRatio = sDesiredAspectRatioSizes[index];
        }
        return aspectRatio;
    }

    private static DecimalFormat sMegaPixelFormat = new DecimalFormat("##0.0");

    /**
     * Given a size return the numerator of its aspect ratio
     *
     * @param size the size to measure
     * @return the numerator
     */
    public static int aspectRatioNumerator(Size size) {
        Size aspectRatio = reduce(size);
        return aspectRatio.getWidth();
    }

    /**
     * Given a size return the numerator of its aspect ratio
     *
     * @param size
     * @return the denominator
     */
    public static int aspectRatioDenominator(Size size) {
        BigInteger width = BigInteger.valueOf(size.getWidth());
        BigInteger height = BigInteger.valueOf(size.getHeight());
        BigInteger gcd = width.gcd(height);
        int denominator = Math.min(width.intValue(), height.intValue()) / gcd.intValue();
        return denominator;
    }

    /**
     * @param size The photo resolution.
     * @return A human readable and translated string for labeling the
     *         picture size in megapixels.
     */
    private String getSizeSummaryString(Size size) {
        Size approximateSize = getApproximateSize(size);
        String megaPixels = sMegaPixelFormat.format((size.getWidth() * size.getHeight()) / 1e6);
        int numerator = aspectRatioNumerator(approximateSize);
        int denominator = aspectRatioDenominator(approximateSize);
        String result =
                getResources().getString(R.string.setting_summary_aspect_ratio_and_megapixels,
                                         numerator, denominator, megaPixels);
        return result;
    }
}