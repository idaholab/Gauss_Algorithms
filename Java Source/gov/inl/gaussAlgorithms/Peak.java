/*
 * Copyright 2017 Battelle Energy Alliance
 */
/*
 *  Gauss Algorithms
 *
 *  File Name: Peak.java
 *
 *  Description: implements a peak & related information
 */
package gov.inl.gaussAlgorithms;

import java.text.NumberFormat;
import java.text.ParseException;

/**
 * describes a peak (either channel or energy)
 *
 */
public class Peak implements Cloneable, Comparable<Peak> {

	private static enum TYPE {
		
		CHANNEL("Channel"),
		ENERGY("Energy");
	
		// member data
		private final String m_label;
	
		TYPE(String label) {
			m_label = label;
		}
		public String label() {
			return m_label;
		}
	};

	protected static final double THRESHOLD = .00001;

	// member data
	private TYPE m_type;
	private double m_channel = 0;
	private boolean m_channelValid = false;
	private double m_energy = 0;
	private boolean m_energyValid = false;
	private double m_sige = 0;
	private boolean m_fixedCentroid = false;

	public Peak(double channel, boolean fixedCentroid) {

		m_type = TYPE.CHANNEL;
		m_channel = channel;
		m_channelValid = true;
		m_fixedCentroid = fixedCentroid;
	}

	public Peak(double energy, double sige, boolean fixedCentroid) {

		m_type = TYPE.ENERGY;
		m_energy = energy;
		m_energyValid = true;
		m_sige = sige;
		m_fixedCentroid = fixedCentroid;
	}

	// used by clone() and JNI
	private Peak(TYPE type, double channel, boolean channelValid,
			double energy, boolean energyValid, double sige,
			boolean fixedCentroid) {

		m_type = type;
		m_channel = channel;
		m_channelValid = channelValid;
		m_energy = energy;
		m_energyValid = energyValid;
		m_sige = sige;
		m_fixedCentroid = fixedCentroid;
	}
		
	// public methods

	public Object clone() {
		return new Peak(m_type, m_channel, m_channelValid, m_energy,
				m_energyValid, m_sige, m_fixedCentroid);
	}

	public int compareTo(Peak other) {

		int answer = 0;

		if (other.m_type == m_type) {
			switch (m_type) {
				case CHANNEL:
					answer = compareDouble(m_channel, other.m_channel);
					break;
				case ENERGY:
					answer = compareDouble(m_energy, other.m_energy);
					break;
				default:
					// let this return equals
					break;
			}
		} else {
			if (m_channelValid && other.m_channelValid) {
				answer = compareDouble(m_channel, other.m_channel);
			} else if (m_energyValid && other.m_energyValid) {
				answer = compareDouble(m_energy, other.m_energy);
			} else {
				// ASSUMPTION: each peak has either a valid channel
				//                               or a valid energy

				// 12/16/2008 idea: set channel zero so energies show first?
				//            idea: could set channel to energy....
				if (m_channelValid) {
					double tempOtherChannel = 0;
					answer = compareDouble(m_channel, tempOtherChannel);
				} else {
					double tempThisChannel = 0;
					answer = compareDouble(tempThisChannel, other.m_channel);
				}
			}
		}
		
		// ignore sige

		return answer;
	}

	public boolean equals(Object peakObject) {

		if (!peakObject.getClass().equals(getClass())) {
			return false;
		}
		Peak other = (Peak) peakObject;

		if (other.m_type != m_type) {
			return false;
		}

		if (other.m_fixedCentroid != m_fixedCentroid) {
			return false;
		}

		boolean answer = true;
		if (0 != compareTo(other)) {
			answer = false;
		}

		// ignore sige

		return answer;
	}

	public double getChannel() {

		return m_channel;
	}

	public double getEnergy() {

		return m_energy;
	}

	public double getSige() {

		return m_sige;
	}

	public int hashCode() {

		int code = m_type.ordinal();
		code = (code * 10) + (m_fixedCentroid ? 0 : 1);
		code = (code * 10) + (m_channelValid ? 0 : 1);
		code = (code * 10) + (m_energyValid ? 0 : 1);
		code = (code * 100) + (int) m_channel;
		code = (code * 100) + (int) m_energy;

		return code;
	}

	public boolean inChannelRange(ChannelRange range) {

		if (m_channelValid && range.contains(m_channel)) {
			return true;
		} else {
			return false;
		}
	}

	public boolean isCentroidFixed() {

		return m_fixedCentroid;
	}

	public boolean isChannelValid() {

		return m_channelValid;
	}

	public boolean isEnergyValid() {

		return m_energyValid;
	}

	public void setFixedCentroid(boolean fixed) {

		m_fixedCentroid = fixed;
	}

	public static double round(double initialValue,
			NumberFormat ofMaxFractionDigits) {

		double answer = initialValue;
		String valueString = ofMaxFractionDigits.format(initialValue);
		try {
			answer = ofMaxFractionDigits.parse(valueString).doubleValue();
		} catch (ParseException pe) {
			// leave default
		}

		return answer;
	}

	public String toString() {
		
		String typeString = m_type.label() + " Peak:";		
		String chanString = "";
		if (m_channelValid) {
			chanString = " channel = " + m_channel + " ";
		}
		String egyString = "";
		if (m_energyValid) {
			egyString = " energy = " + m_energy + " uncert = " + m_sige + " ";
		}
		String fixString = "";
		if (m_fixedCentroid) {
			fixString = " FIXED ";
		} else {
			fixString = " not fixed ";
			}
		return typeString + chanString + egyString + fixString;
	}

	public void undefineEnergy() {

		switch(m_type) {
			case CHANNEL:
				m_energyValid = false;
				break;
			case ENERGY:
				m_channelValid = false;
				break;
			default:
				break;
		}
    }

	public void update(EnergyEquation ex) {

		switch(m_type) {
			case CHANNEL:
				try {
					m_energy = ex.getEnergy(m_channel);
					m_sige = 0;
					m_energyValid = true;
				} catch (Exception e) {
					m_energyValid = false;
				}
				break;
			case ENERGY:
				try {
					m_channel = ex.getChannel(m_energy);
					m_channelValid = true;
				} catch (Exception e) {
					m_channelValid = false;
				}
				break;
			default:
				break;
		}
	}

	public void update(EnergyEquation ex, NumberFormat ofMaxFractionDigits) {

		switch(m_type) {
			case CHANNEL:
				try {
					m_energy = ex.getEnergy(m_channel);
					m_energy = round(m_energy, ofMaxFractionDigits);
					m_sige = 0;
					m_energyValid = true;
				} catch (Exception e) {
					m_energyValid = false;
				}
				break;
			case ENERGY:
				try {
					m_channel = ex.getChannel(m_energy);
					m_channel = round(m_channel, ofMaxFractionDigits);
					m_channelValid = true;
				} catch (Exception e) {
					m_channelValid = false;
				}
				break;
			default:
				break;
		}
	}

	// private methods

	private int compareDouble(double fromThisPeak, double fromOtherPeak) {

		int answer = 0;

		double diff = Math.abs(fromThisPeak - fromOtherPeak);
		if (diff > THRESHOLD) {
			answer = Double.compare(fromThisPeak, fromOtherPeak);
		}

		return answer;
	}
}
