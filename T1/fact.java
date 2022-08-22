import java.math.BigInteger;
import java.util.Random;

public class fact {
  public static void main(String args[]) {
    BigInteger f= bigFact(Integer.parseInt(args[0]));
    String hex= f.toString(16);
    System.out.println(f.toString(16));
  }

  static BigInteger simpleBigFact(int n) {
    BigInteger bign= BigInteger.valueOf(n);
    BigInteger f= BigInteger.ONE;
    BigInteger i= BigInteger.ONE;
    while (i.compareTo(bign)<=0) {
      f = f.multiply(i);
      i = i.add(BigInteger.ONE);
    }
    return f;
  }

  static BigInteger bigFact(int n) {
    int a[]= new int[n];
    for (int i= 0; i<n; i++)
      a[i]= i+1;
    Random rand= new Random();
    for (int i= 0; i<n; i++) {
      int r= rand.nextInt(n-i)+i;
      int tmp= a[i];
      a[i]= a[r];
      a[r]= tmp;
    }
    return bigArrayProd(a, 0, n-1);
  }

  static BigInteger bigArrayProd(int a[], int i, int j) {
    if (i>j) {
      System.err.println("Assertion failure\n");
      System.exit(1);
    }
    if (i==j) {
      return BigInteger.valueOf(a[i]);
    }
    else {
      int h= (i+j)/2;
       BigInteger left= bigArrayProd(a, i, h);
       BigInteger right= bigArrayProd(a, h+1, j);
       return left.multiply(right);
    }
  }
}
