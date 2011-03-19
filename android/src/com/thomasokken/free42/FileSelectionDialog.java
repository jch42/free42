package com.thomasokken.free42;

import java.io.File;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.StringTokenizer;

import android.app.Dialog;
import android.content.Context;
import android.database.DataSetObserver;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Adapter;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ImageView;
import android.widget.ListAdapter;
import android.widget.ListView;
import android.widget.RelativeLayout;
import android.widget.Spinner;
import android.widget.TextView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.AdapterView.OnItemSelectedListener;

public class FileSelectionDialog extends Dialog {
	private FileSelView view;
	private Spinner dirListSpinner;
	private ListView dirView;
	private Spinner fileTypeSpinner;
	private EditText fileNameTF;
	private Button upButton;
	private Button mkdirButton;
	private Button okButton;
	private Button cancelButton;
	private OkListener okListener;
	private String currentPath;
	
	public FileSelectionDialog(Context ctx, String[] types) {
		super(ctx);
		view = new FileSelView(ctx, types);
		setContentView(view);
		setTitle("Select File");
		setPath("/");
	}
	
	public interface OkListener {
		public void okPressed(String path);
	}

	public void setOkListener(OkListener okListener) {
		this.okListener = okListener;
	}
	
	public void setPath(String path) {
		String fileName = "";
		File f = new File(path);
		if (!f.exists() || f.isFile()) {
			int p = path.lastIndexOf("/");
			if (p == -1) {
				fileName = path;
				path = "/";
			} else {
				fileName = path.substring(p + 1);
				path = path.substring(0, p);
				if (path.length() == 0)
					path = "/";
			}
		}
		StringTokenizer tok = new StringTokenizer(path, "/");
		List<String> pathComps = new ArrayList<String>();
		pathComps.add("/");
		StringBuffer pathBuf = new StringBuffer();
		pathBuf.append("/");
		while (tok.hasMoreTokens()) {
			String t = tok.nextToken();
			pathComps.add(t);
			pathBuf.append(t);
			pathBuf.append("/");
		}
		ArrayAdapter<String> aa = new ArrayAdapter<String>(getContext(), android.R.layout.simple_spinner_item, pathComps);
		dirListSpinner.setAdapter(aa);
		dirListSpinner.setSelection(pathComps.size() - 1);
		fileNameTF.setText(fileName);
		currentPath = pathBuf.toString();
		
		File[] list = new File(currentPath).listFiles();
		if (list == null)
			list = new File[0];
		Arrays.sort(list);
		String type = (String) fileTypeSpinner.getSelectedItem();
		if (type.equals("*"))
			type = null;
		dirView.setAdapter(new DirListAdapter(list, type));
	}

	private void doUp() {
		if (currentPath.length() > 1) {
			int n = currentPath.lastIndexOf("/", currentPath.length() - 2);
			setPath(currentPath.substring(0, n + 1) + fileNameTF.getText().toString());
		}
	}
	
	private void doMkDir() {
		// TODO
	}
	
	private class FileSelView extends RelativeLayout {
		public FileSelView(Context ctx, String[] types) {
			super(ctx);
			
			int id = 1;
			
			dirListSpinner = new Spinner(ctx);
			dirListSpinner.setId(id++);
			dirListSpinner.setOnItemSelectedListener(new OnItemSelectedListener() {
				public void onItemSelected(AdapterView<?> view, View parent, int position, long id) {
					Adapter a = view.getAdapter();
					if (position == a.getCount() - 1)
						return;
					StringBuffer pathBuf = new StringBuffer("/");
					for (int i = 1; i <= position; i++) {
						pathBuf.append(a.getItem(i));
						pathBuf.append("/");
					}
					setPath(pathBuf.toString());
				}
				public void onNothingSelected(AdapterView<?> arg0) {
					// Shouldn't happen
				}
			});
			RelativeLayout.LayoutParams lp = new RelativeLayout.LayoutParams(RelativeLayout.LayoutParams.WRAP_CONTENT, RelativeLayout.LayoutParams.WRAP_CONTENT);
			lp.addRule(RelativeLayout.ALIGN_PARENT_TOP);
			lp.addRule(RelativeLayout.ALIGN_PARENT_LEFT);
			addView(dirListSpinner, lp);

			fileTypeSpinner = new Spinner(ctx);
			fileTypeSpinner.setId(id++);
			ArrayAdapter<String> aa = new ArrayAdapter<String>(getContext(), android.R.layout.simple_spinner_item, types);
			fileTypeSpinner.setAdapter(aa);
			fileTypeSpinner.setOnItemSelectedListener(new OnItemSelectedListener() {
				public void onItemSelected(AdapterView<?> view, View parent, int position, long id) {
					String type = (String) view.getAdapter().getItem(position);
					if (type.equals("*"))
						type = null;
					DirListAdapter dla = (DirListAdapter) dirView.getAdapter();
					if (dla != null)
						dla.setType(type);
				}
				public void onNothingSelected(AdapterView<?> arg0) {
					// Shouldn't happen
				}
			});
			lp = new RelativeLayout.LayoutParams(RelativeLayout.LayoutParams.WRAP_CONTENT, RelativeLayout.LayoutParams.WRAP_CONTENT);
			lp.addRule(RelativeLayout.ALIGN_PARENT_BOTTOM);
			lp.addRule(RelativeLayout.ALIGN_PARENT_LEFT);
			addView(fileTypeSpinner, lp);

			fileNameTF = new EditText(ctx);
			fileNameTF.setId(id++);
			lp = new RelativeLayout.LayoutParams(RelativeLayout.LayoutParams.WRAP_CONTENT, RelativeLayout.LayoutParams.WRAP_CONTENT);
			lp.addRule(RelativeLayout.ALIGN_PARENT_BOTTOM);
			lp.addRule(RelativeLayout.ALIGN_PARENT_RIGHT);
			lp.addRule(RelativeLayout.RIGHT_OF, fileTypeSpinner.getId());
			addView(fileNameTF, lp);

			RelativeLayout buttonBox = new RelativeLayout(ctx);
			buttonBox.setId(id++);
			upButton = new Button(ctx);
			upButton.setId(id++);
			upButton.setText("Up");
			upButton.setOnClickListener(new OnClickListener() {
				public void onClick(View view) {
					doUp();
				}
			});
			lp = new RelativeLayout.LayoutParams(RelativeLayout.LayoutParams.WRAP_CONTENT, RelativeLayout.LayoutParams.WRAP_CONTENT);
			lp.addRule(RelativeLayout.ALIGN_PARENT_TOP);
			lp.addRule(RelativeLayout.CENTER_HORIZONTAL);
			buttonBox.addView(upButton, lp);
			mkdirButton = new Button(ctx);
			mkdirButton.setId(id++);
			mkdirButton.setText("MkDir");
			mkdirButton.setOnClickListener(new OnClickListener() {
				public void onClick(View view) {
					doMkDir();
				}
			});
			lp = new RelativeLayout.LayoutParams(RelativeLayout.LayoutParams.WRAP_CONTENT, RelativeLayout.LayoutParams.WRAP_CONTENT);
			lp.addRule(RelativeLayout.BELOW, upButton.getId());
			lp.addRule(RelativeLayout.CENTER_HORIZONTAL);
			buttonBox.addView(mkdirButton, lp);
			okButton = new Button(ctx);
			okButton.setId(id++);
			okButton.setText("OK");
			okButton.setOnClickListener(new OnClickListener() {
				public void onClick(View view) {
					if (okListener != null)
						okListener.okPressed(currentPath + fileNameTF.getText().toString());
					FileSelectionDialog.this.dismiss();
				}
			});
			lp = new RelativeLayout.LayoutParams(RelativeLayout.LayoutParams.WRAP_CONTENT, RelativeLayout.LayoutParams.WRAP_CONTENT);
			lp.addRule(RelativeLayout.BELOW, mkdirButton.getId());
			lp.addRule(RelativeLayout.CENTER_HORIZONTAL);
			buttonBox.addView(okButton, lp);
			cancelButton = new Button(ctx);
			cancelButton.setId(id++);
			cancelButton.setText("Cancel");
			cancelButton.setOnClickListener(new OnClickListener() {
				public void onClick(View view) {
					FileSelectionDialog.this.dismiss();
				}
			});
			lp = new RelativeLayout.LayoutParams(RelativeLayout.LayoutParams.WRAP_CONTENT, RelativeLayout.LayoutParams.WRAP_CONTENT);
			lp.addRule(RelativeLayout.BELOW, okButton.getId());
			lp.addRule(RelativeLayout.CENTER_HORIZONTAL);
			buttonBox.addView(cancelButton, lp);
			lp = new RelativeLayout.LayoutParams(RelativeLayout.LayoutParams.WRAP_CONTENT, RelativeLayout.LayoutParams.WRAP_CONTENT);
			lp.addRule(RelativeLayout.BELOW, dirListSpinner.getId());
			lp.addRule(RelativeLayout.ABOVE, fileTypeSpinner.getId());
			lp.addRule(RelativeLayout.ALIGN_PARENT_RIGHT);
			addView(buttonBox, lp);

			dirView = new ListView(ctx);
			dirView.setId(id++);
			dirView.setOnItemClickListener(new OnItemClickListener() {
				public void onItemClick(AdapterView<?> view, View parent, int position, long id) {
					File item = (File) view.getAdapter().getItem(position);
					if (item.isDirectory())
						setPath(item.getAbsolutePath());
					else
						fileNameTF.setText(item.getName());
				}
			});
			lp = new RelativeLayout.LayoutParams(RelativeLayout.LayoutParams.WRAP_CONTENT, RelativeLayout.LayoutParams.WRAP_CONTENT);
			lp.addRule(RelativeLayout.BELOW, dirListSpinner.getId());
			lp.addRule(RelativeLayout.ABOVE, fileTypeSpinner.getId());
			lp.addRule(RelativeLayout.ALIGN_PARENT_LEFT);
			lp.addRule(RelativeLayout.LEFT_OF, buttonBox.getId());
			addView(dirView, lp);
		}
	}
	
	private static class DirListAdapter implements ListAdapter {
		private File[] items;
		private String type;
		private List<DataSetObserver> observers = new ArrayList<DataSetObserver>();
		
		public DirListAdapter(File[] items, String type) {
			this.items = items;
			this.type = type;
		}
		
		public void setType(String type) {
			boolean changed = type == null ? this.type != null : !type.equals(this.type);
			this.type = type;
			if (changed) {
				DataSetObserver[] dsoArray;
				synchronized (observers) {
					dsoArray = observers.toArray(new DataSetObserver[observers.size()]);
				}
				for (DataSetObserver dso : dsoArray)
					dso.onChanged();
			}
		}

		@Override
		public int getCount() {
			return items.length;
		}

		@Override
		public Object getItem(int position) {
			return items[position];
		}

		@Override
		public long getItemId(int position) {
			return position;
		}

		@Override
		public int getItemViewType(int position) {
			return items[position].isDirectory() ? 0 : 1;
		}

		@Override
		public View getView(int position, View convertView, ViewGroup parent) {
			File item = items[position];
			if (convertView == null) {
				Context context = parent.getContext();
				RelativeLayout view = new RelativeLayout(context);
    			ImageView icon = new ImageView(context);
    			icon.setId(1);
    			icon.setImageResource(item.isDirectory() ? R.drawable.folder : R.drawable.document);
    			RelativeLayout.LayoutParams lp = new RelativeLayout.LayoutParams(RelativeLayout.LayoutParams.WRAP_CONTENT, RelativeLayout.LayoutParams.WRAP_CONTENT);
    			lp.addRule(RelativeLayout.ALIGN_PARENT_TOP);
    			lp.addRule(RelativeLayout.ALIGN_PARENT_LEFT);
    			lp.addRule(RelativeLayout.ALIGN_PARENT_BOTTOM);
    			view.addView(icon, lp);
    			TextView text = new TextView(context);
    			text.setId(2);
    			text.setText(item.getName());
    			lp = new RelativeLayout.LayoutParams(RelativeLayout.LayoutParams.WRAP_CONTENT, RelativeLayout.LayoutParams.WRAP_CONTENT);
    			lp.addRule(RelativeLayout.ALIGN_PARENT_TOP);
    			lp.addRule(RelativeLayout.ALIGN_PARENT_RIGHT);
    			lp.addRule(RelativeLayout.ALIGN_PARENT_BOTTOM);
    			lp.addRule(RelativeLayout.RIGHT_OF, icon.getId());
    			view.addView(text, lp);
    			return view;
			} else {
				RelativeLayout view = (RelativeLayout) convertView;
				TextView text = (TextView) view.getChildAt(1);
				text.setText(item.getName());
				return view;
			}
		}

		@Override
		public int getViewTypeCount() {
			return 2;
		}

		@Override
		public boolean hasStableIds() {
			return true;
		}

		@Override
		public boolean isEmpty() {
			return items.length == 0;
		}

		@Override
		public void registerDataSetObserver(DataSetObserver observer) {
			synchronized (observers) {
				observers.add(observer);
			}
		}

		@Override
		public void unregisterDataSetObserver(DataSetObserver observer) {
			synchronized (observers) {
				observers.add(observer);
			}
		}

		@Override
		public boolean areAllItemsEnabled() {
			return false;
		}

		@Override
		public boolean isEnabled(int position) {
			File item = items[position];
			return item.isDirectory() || type == null || item.getName().endsWith("." + type);
		}
	}
}