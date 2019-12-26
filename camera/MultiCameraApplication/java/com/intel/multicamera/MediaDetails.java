/*
 * Copyright (C) 2013 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.intel.multicamera;

import android.content.Context;
import android.provider.MediaStore;
import android.util.Size;
import android.util.SparseIntArray;
import java.util.Iterator;
import java.util.Map.Entry;
import java.util.TreeMap;

public class MediaDetails implements Iterable<Entry<Integer, Object>> {
    @SuppressWarnings("unused") private static final String TAG = "MediaDetails";

    private TreeMap<Integer, Object> mDetails = new TreeMap<Integer, Object>();
    private SparseIntArray mUnits = new SparseIntArray();
    public static final int INDEX_TYPE = 0;
    public static final int INDEX_TITLE = 1;
    public static final int INDEX_DESCRIPTION = 2;
    public static final int INDEX_DATETIME = 3;
    public static final int INDEX_DATEMODIFIED = 4;
    public static final int INDEX_LOCATION = 5;
    public static final int INDEX_WIDTH = 6;
    public static final int INDEX_HEIGHT = 7;
    public static final int INDEX_DIMENSIONS = 8;
    public static final int INDEX_ORIENTATION = 9;
    public static final int INDEX_DURATION = 10;
    public static final int INDEX_MIMETYPE = 11;
    public static final int INDEX_SIZE = 12;

    // for EXIF
    public static final int INDEX_MAKE = 100;
    public static final int INDEX_MODEL = 101;

    // Put this last because it may be long.
    public static final int INDEX_PATH = 200;

    public void addDetail(int index, Object value) {
        mDetails.put(index, value);
    }

    public Object getDetail(int index) {
        return mDetails.get(index);
    }

    public int size() {
        return mDetails.size();
    }

    @Override
    public Iterator<Entry<Integer, Object>> iterator() {
        return mDetails.entrySet().iterator();
    }

    public void setUnit(int index, int unit) {
        mUnits.put(index, unit);
    }

    public boolean hasUnit(int index) {
        return mUnits.indexOfKey(index) >= 0;
    }

    public int getUnit(int index) {
        return mUnits.get(index);
    }

    /**
     * Returns a (localized) string for the given duration (in seconds).
     */
    public static String formatDuration(final Context context, long seconds) {
        long h = seconds / 3600;
        long m = (seconds - h * 3600) / 60;
        long s = seconds - (h * 3600 + m * 60);
        String durationValue;
        if (h == 0) {
            durationValue = String.format(context.getString(R.string.details_ms), m, s);
        } else {
            durationValue = String.format(context.getString(R.string.details_hms), h, m, s);
        }
        return durationValue;
    }

    public static String getDimentions(final Context context, int INDEX_WIDTH, int INDEX_HEIGHT) {
        float megaPixels = INDEX_HEIGHT * INDEX_WIDTH / 1000000f;
        Size size = new Size(INDEX_WIDTH, INDEX_HEIGHT);
        int numerator = SettingsPrefUtil.aspectRatioNumerator(size);
        int denominator = SettingsPrefUtil.aspectRatioDenominator(size);

        return context.getString(R.string.metadata_dimensions_format, INDEX_WIDTH, INDEX_HEIGHT,
                                 megaPixels, numerator, denominator);
    }
}
