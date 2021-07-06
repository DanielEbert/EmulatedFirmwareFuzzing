int arr[2];
void shift() {arr[1] = arr[0];}

void push(int *p) {
  shift();
  arr[0] = *p;
}

int pop() {
  int x = arr[1];
  shift();
  return x;
}

void func1() {
  int local_var;
  push(&local_var);
}
void setup() {
  func1();
  shift();
  int i = pop();
}

void myFunction() {

}

void loop(){}
