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

import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.content.Context;
import android.content.DialogInterface;
import android.text.format.Formatter;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.ListView;
import android.widget.TextView;

public class USBChangeDialog {
    /**
     * Creates a dialog to show USB hot plug message
     *
     * @param context the Android context.
     * @return A dialog that can be made visible to show the media details.
     */
    public static Dialog create(final Context context) {
        ListView detailsList =
                (ListView)LayoutInflater.from(context).inflate(R.layout.details_list, null, false);
        detailsList.setAdapter(new DetailsAdapter(context));

        final AlertDialog.Builder builder = new AlertDialog.Builder(context);
        return builder.setTitle(R.string.usb_change)
                .setView(detailsList)
                .setPositiveButton(R.string.close,
                                   new DialogInterface.OnClickListener() {
                                       @Override
                                       public void onClick(DialogInterface dialog,
                                                           int whichButton) {
                                           Activity activity = (Activity) context;
                                           activity.finish();
                                       }
                                   })
                .create();
    }

    /**
     * An adapter for feeding a details list view.
     */
    private static class DetailsAdapter extends BaseAdapter {
        private final Context mContext;

        public DetailsAdapter(Context context) {
            mContext = context;
        }

        @Override
        public boolean areAllItemsEnabled() {
            return false;
        }

        @Override
        public boolean isEnabled(int position) {
            return false;
        }

        @Override
        public int getCount() {
            return 0;
        }

        @Override
        public Object getItem(int position) {
            return null;
        }

        @Override
        public long getItemId(int position) {
            return position;
        }

        @Override
        public View getView(int position, View convertView, ViewGroup parent) {
            TextView tv;
            if (convertView == null) {
                tv = (TextView)LayoutInflater.from(mContext).inflate(R.layout.details, parent,
                                                                     false);
            } else {
                tv = (TextView)convertView;
            }
            return tv;
        }
    }
}
