import sjvm.*;

public class javatest {
	public static void main(String[] args) {
		int a = 0;
		int b = 5;
		double c = a + b;
		SJVM.nativePrint(c == 5 ? "true" : "false");	
	}
}
