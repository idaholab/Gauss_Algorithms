/*
 * Copyright 2017 Battelle Energy Alliance
 */
/*
 *  Gauss Algorithms
 *
 *  File Name: EnergyEquation.java
 *
 *  Description: Implements the energy equation
 */
package gov.inl.gaussAlgorithms;

/**
 * the energy equation contains the energy calibration
 *
 */
public class EnergyEquation implements Cloneable {
	
	/**
	 * this indicates the energy equation form:
	 * e(x) = a + b*x
	 * or
	 * e(x) = a + b*x + c*x^2
	 *
	 */
	public static enum MODE {
		
		LINEAR("linear"),
		QUADRATIC("quadratic");
	
		// member data
		private final String m_label;
	
		MODE(String label) {
			m_label = label;
		}
		public String label() {
			return m_label;
		}
	};

	/*
	 * this holds coefficients for the energy equation
	 * e(x) = a + b*x + c*x**2
	 * and chi squared from corresponding calibration.
	 */
	private final double m_a;
	private final double m_b;
	private final double m_c;
	private final double m_chi_sq;
	private final MODE m_mode;

	public EnergyEquation(double constCoeff, double linCoeff, double quadCoeff,
						  double chi_sq, MODE mode) {

		m_a = constCoeff;
		m_b = linCoeff;
		m_c = quadCoeff;
		m_chi_sq = chi_sq;
		m_mode = mode;
	}

	public EnergyEquation clone() {
		
		EnergyEquation eqn = new EnergyEquation(m_a, m_b, m_c, m_chi_sq,
				m_mode);

		return eqn;
	}
	
	public boolean equals(Object other) {
		
		if (null == other) {
			return false;
		}
		
		if (!(other instanceof EnergyEquation)) {
			return false;
		}
		
		boolean answer = false;
		
		// NOTE: chi squared currently ignored. no java property for it.
		
		EnergyEquation oex = (EnergyEquation) other;
		
		if ((m_a == oex.m_a) && (m_b == oex.m_b) && (m_mode == oex.m_mode)) {
			if (MODE.QUADRATIC.equals(m_mode)) {
				if (m_c == oex.m_c) {
					answer = true;
				}
			} else {
				answer = true;
			}
		}
		
		return answer;
	}

	public double getChannel(double energy) throws Exception{
				
		double channel = 0;
		
		if (EnergyEquation.MODE.LINEAR.equals(m_mode) || (m_c == 0.0)) {
			if (m_b == 0) {
				throw new Exception("energy is constant");
			} else {
				channel = (energy - m_a) / m_b;
			}
		} else {
			double temp = (m_b * m_b) - (4 * m_c * (m_a - energy));
			
			if (temp < 0) {
				throw new Exception("b^2-4ac is negative");
			} else {
				channel = (Math.sqrt(temp) - m_b) / (2 * m_c);
				channel = Math.max(0, channel);
			}
		}

		return channel;
	}

	public double getChiSq() {
		return m_chi_sq;
	}

	public double getConstantCoefficient() {
		return m_a;
	}

	public double getEnergy(double channel) {
		
		double energy = m_a + (m_b * channel);
		
		if (EnergyEquation.MODE.QUADRATIC.equals(m_mode)) {
			energy += m_c * channel * channel;
		}

		return energy;
	}

	public double getLinearCoefficient() {
		return m_b;
	}
	
	public MODE getMode() {
		return m_mode;
	}

	public double getQuadCoefficient() {
		return m_c;
	}
	
	public String toDisplayString() {
		
		String bSign = " ";
		String cSign = " ";
		
		if (0 <= m_b) {
			bSign = " +";
		}
		if (0 <= m_c) {
			cSign = " +";
		}
		
		if (MODE.LINEAR.equals(m_mode) || (0 == m_c)) {
			return "e(x) = " + m_a + bSign + m_b + "X";
		} else {
			return "e(x) = " + m_a + bSign + m_b + "X " + cSign + m_c + "X2";
		}
	}

	public String toString() {
				
		return "e(x) = " + m_a + " + " + m_b + "X + " + m_c + "X2 (mode = " +
				m_mode.label() + "; chisq = " + m_chi_sq + ")";
	}
}
