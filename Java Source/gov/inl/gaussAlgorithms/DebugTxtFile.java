/*
 * Copyright 2017 Battelle Energy Alliance
 */
/*
 *  Gauss Algorithms
 *
 *  File Name: DebugTxtFile.java
 *
 *  Description: Implements reading and writing text files for debugging
 */
package gov.inl.gaussAlgorithms;

import java.io.BufferedWriter;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.OutputStreamWriter;
import java.nio.charset.Charset;
import java.nio.charset.IllegalCharsetNameException;
import java.util.Scanner;
import java.util.Vector;

public class DebugTxtFile {
	
	public final static String    FILE_SUFFIX = "txt";
	
	public final static String    LINE_SEPARATOR;
	static {
		LINE_SEPARATOR = System.getProperty("line.separator", "\n");
	}
	public final static char      TAB_CHARACTER = '\t';
	public final static String    WHITESPACE_DELIM = "[\\p{javaWhitespace}]+";

	
	// UTC-16 encoding for HTML found on 1/13/2010 at
	// http://en.wikipedia.org/wiki/List_of_XML_and_HTML_character_entity_references
	//private final static String SIGMA_HTML = "&#x03C3";
	public final static char      SIGMA_CHARACTER = 0x03C3;
	public final static char      SUP2_CHARACTER = 0x00B2;
	public final static char      SQUARED_CHARACTER = SUP2_CHARACTER;
	public final static char      SQUARE_ROOT_CHARACTER = 0x221A;
	public final static char      CAPITAL_CHI_CHARACTER = 0x03A7;
	public final static char      DELTA_CHARACTER = 0x0394;
		
	// member data
	
	private FileOutputStream      m_foStream = null;
	private OutputStreamWriter    m_osWriter = null;
	private BufferedWriter        m_bWriter = null;
	
	private FileInputStream       m_inStream = null;
	private Scanner               m_scanner = null;
	
	private File                  m_txtFile = null;
	// 1/14/2010 MS Excel 2007 knows UTF-8, not UTF-16
	private String                m_charsetName = "UTF-8";

	// constructors
	
	public DebugTxtFile(File fullPathName) {
		
		this(fullPathName, true, "");
	}
	
	public DebugTxtFile(File fullPathName, boolean useTxtSuffix) {
		
		this(fullPathName, useTxtSuffix, "");
	}
	
	public DebugTxtFile(File fullPathName, String unicodeFormatName) {
		
		this(fullPathName, true, unicodeFormatName);
	}
	
	public DebugTxtFile(File fullPathName, boolean useTxtSuffix,
			String unicodeFormatName) {
		
		init(fullPathName, useTxtSuffix, unicodeFormatName);
	}
	
	// public methods
	
	public void append(String results) throws IOException {
		
		openToWriteFile(true);
		m_bWriter.write(results);
		m_bWriter.newLine();
		closeTxtFile();
	}
	
	public void append(String[] results) throws IOException {
		
		openToWriteFile(true);
		int lineCount = results.length;
		for (int i = 0; i < lineCount; i++) {
			m_bWriter.write(results[i]);
			m_bWriter.newLine();
		}
		closeTxtFile();
	}
	
	public boolean exists() {
		
		return m_txtFile.exists();
	}
	
	public Vector<String> read() throws IOException {
		
		openToReadFile();
		
		Vector<String> lines = new Vector<String>(32, 32);
		while (m_scanner.hasNextLine()) {
			String line = m_scanner.nextLine();
			lines.add(line);
		}
		
		return lines;
	}
	
	public void write(String results) throws IOException {
		
		openToWriteFile(false);
		m_bWriter.write(results);
		m_bWriter.newLine();
		closeTxtFile();
	}
	
	public void write(String[] results) throws IOException {
		
		openToWriteFile(false);
		int lineCount = results.length;
		for (int i = 0; i < lineCount; i++) {
			m_bWriter.write(results[i]);
			m_bWriter.newLine();
		}
		closeTxtFile();
	}
	
	// private methods
	
	private void closeTxtFile() throws IOException {
		
		if (null != m_bWriter) {
			m_bWriter.close();
		}
		m_bWriter = null;		
		if (null != m_osWriter) {
			m_osWriter.close();
		}
		m_osWriter = null;
		if (null != m_foStream) {
			m_foStream.close();
		}
		m_foStream = null;

		if (null != m_scanner) {
			m_scanner.close();
		}
		if (null != m_inStream) {
			m_inStream.close();
		}
		m_inStream = null;
	}
	
	private void init(File fullPathName, boolean useTxtSuffix,
			String unicodeFormatName) {
		
		m_txtFile = fullPathName;
		
		String selectedName = fullPathName.getName();
		if (useTxtSuffix) {
			if (!selectedName.endsWith("." + FILE_SUFFIX)) {
				String newName = fullPathName.getPath() +
								"." + FILE_SUFFIX;
				m_txtFile = new File(newName);
			}
		}
		
		try {
			if (Charset.isSupported(unicodeFormatName)) {
				m_charsetName = unicodeFormatName;
			}
		} catch (IllegalCharsetNameException e) {
			// use default
		}
	}
	
	private void openToReadFile() throws FileNotFoundException {
		
		m_inStream = new FileInputStream(m_txtFile);
		m_scanner = new Scanner(m_inStream, m_charsetName);
	}
	
	private void openToWriteFile(boolean append) throws IOException {
		
		m_foStream = new FileOutputStream(m_txtFile, append);
		m_osWriter = new OutputStreamWriter(m_foStream, m_charsetName);
		m_bWriter = new BufferedWriter(m_osWriter);
	}
}
